//
//  Parallel_GC_Thread.cpp
//  RoarVM
//
//  Created by Mattias De Wael on 26/07/11.
//  Copyright 2011 VUB. All rights reserved.
//

#include <pthread.h>
#include "headers.h"

// Set all checks to zero in production build.
#define DO_SIMPLE_CHECKPOINT_CHECK     0
#define DO_NMT_SETTING_CHECK           0
#define DO_ALLOCATION_IN_GC_HEAP_CHECK 0
#define DO_PROTECTED_PAGE_CHECK        0
#define DO_FIRST_OBJECT_OF_PAGE_TEST   0
#define DO_RELOCATE_TEST               1

#define VERBOSE_CHECKPOINTS            1


void GC_Thread_Class::do_work()
{
  
  Thread_Memory_Semantics::initialize_corekey_for_GC();
  printf("GC_thread running (%d)...\n", Logical_Core::my_rank());
  The_Memory_System()->create_heap_for_GC();
  
  Logical_Core::my_core()->setNMT(false);
  //install_signalhandler_protectedPageAcces();
  
  Memory_Semantics::initialize_local_interpreter_GC( m_initialInterpreter );
  
  
  if(DO_ALLOCATION_IN_GC_HEAP_CHECK) {
    The_Memory_System()->my_heap()->allocateChunk(123);
  }
  
  if(DO_NMT_SETTING_CHECK) {
    bool nmt = m_initialInterpreter->roots.nilObj.getNMT();
    
    assert_always(m_initialInterpreter->roots.nilObj.is_mem());
    Object* nil1 = m_initialInterpreter->roots.nilObj.as_object();
    m_initialInterpreter->roots.nilObj.setNMT(!m_initialInterpreter->roots.nilObj.getNMT());
    assert_always(m_initialInterpreter->roots.nilObj.is_mem() && m_initialInterpreter->roots.nilObj.getNMT() == !nmt);
    Object* nil2 = m_initialInterpreter->roots.nilObj.as_object();
    assert_always(nil1 == nil2);
    m_initialInterpreter->roots.nilObj.setNMT(!m_initialInterpreter->roots.nilObj.getNMT());
    assert_always(m_initialInterpreter->roots.nilObj.is_mem() && m_initialInterpreter->roots.nilObj.getNMT() == nmt);
  }
  
  if(DO_SIMPLE_CHECKPOINT_CHECK){
    checkpoint_simple();
  }
  
  if(DO_PROTECTED_PAGE_CHECK)
    TEST_force_protectedPage_signal_trap();
    
  
  if(DO_FIRST_OBJECT_OF_PAGE_TEST){
    Page* p = The_Memory_System()->allocate(1);
    p->firstObject();
    
    FOR_EACH_PAGE(page){
      int pageNbr = page->pageNumber();
      Object* o = page->firstObject();
      printf("page %d\t%p\n",pageNbr,o);
    }
  }
  
  if( DO_RELOCATE_TEST ) {
    
  }
  
  printf("Check if all cores are yet interpreting...\n");
  //Message_Statics::receive_and_handle_all_checkpoint_responses_GC(Message_Statics::checkpoint_startInterpretation_Message);
  printf("GC now knows all cores are interpreting... start the real work\n");
  while (!m_stoprequested)
  {
     phase_mark();
     phase_relocate();
     phase_remap();
    temp_GC_ISREADY_Message_class msg;
    msg.send_to( 0 );
    m_stoprequested = true;
  }
  
}

/***************************************************
 *                         MARK                    *
 ***************************************************/

void GC_Thread_Class::phase_mark(){
  phase->set_phase_mark();
  assert( is_mark_phase() );
  
  printf("GC\tInit datastructures\n");
  initInternalDataStructuresForMark();
  
  bool oldNMT = Logical_Core::my_NMT();
  printf("GC\tFlip NMT (NMT is now %s)\n", oldNMT?"T":"F");
  flipNMT(); // Select a new value for NMT.
  printf("GC\t NMT flipped, is now %s\n", Logical_Core::my_NMT()?"T":"F");
  assert(oldNMT != Logical_Core::my_NMT());
  
  printf("GC\tDo synchronisation(Adress of the memory system=%p)\n",The_Memory_System());
  The_Memory_System()->startGCpreparation();
  printf("GC\tMemory system notified.\n");
  checkpoint_startMark();
  m_pageLiveness = The_Memory_System()->stopGCpreparation();
  printf("GC\tSynchronised, start marking...\n");
  
  printLivenessArray( m_pageLiveness );
  
  int oc = 0;
  FOR_EACH_OBJECT(obj){
    if( !obj->is_free() ){
      oc++;
    }
  }
  printf("non-free objects:(%d)\n",oc);
  
  
  int markCount= 0;
  int c1 =0;
  int c2 =0;
  int c3 =0;
  
  // This loop corresponds to the finishmark CP, and is needed to handle possibly added NTM-trapped refs.
  // Just before a mutator thread passes its checkpoint, it sends its NTM-trapped refs.
  while( !mark_stack_.is_empty()){ 
    
    // This loop corresponds to the receiving of intermediate added NTM-trapped refs.
    // Every bulk of 10k NMT trapped refs are send to to the GC-thread.
    while( !mark_stack_.is_empty()){
      
      // This loop corresponds to the trivial emptying of the mark-stack.
      while( !mark_stack_.is_empty()){
        // Pop one at the time and traverse.
        Object* object = mark_stack_.pop();
        if(object != NULL && !object->is_marked() ) {
          
          if(mark_traverse(object)){
            markCount++;
          }
        }
        c1++;
      }
      // We anticipate on receiving report_bulk_NMTtrapped_refs_Messages to have arrived.
      Message_Statics::process_any_incoming_messages_as_GC(false);
      c2++;
    }
    // Force all mutator-threads to push their current NMT-trapped-refs to the stack.
    checkpoint_finishMark();
    c3++;
  } //Restart if new objects are added.
  printf("Finished mark phase (did %d marks)(%d %d %d)...\n",markCount,c1,c2,c3);
  printLivenessArray( m_pageLiveness );
  
  phase->unset_phase_mark();
  assert( !is_mark_phase() );
}

void GC_Thread_Class::initInternalDataStructuresForMark(){
  initMarkStack();
}

void GC_Thread_Class::checkpoint_startMark(){
  printf("Start new mark phase with %s as NMT value...\n",Logical_Core::my_NMT()?"T":"F");
  checkpoint_startMark_Message_class m( Logical_Core::my_NMT() );
  doCheckpoint(&m, Message_Statics::checkpoint_startMark_Response);
  /* Mutator-threads mark their roots-set with new NMT and reply with the refs in their root set. */
  verbosePrint_checkpoint("Passed start-mark checkpoint...");
}

void GC_Thread_Class::addRoots( GC_Oop_Stack* aRootStack ){
  mark_stack_.pushOtherStack( aRootStack );
}

void GC_Thread_Class::checkpoint_finishMark(){
  verbosePrint_checkpoint("Start finish-mark checkpoint...");
  checkpoint_finishMark_Message_class m;
  doCheckpoint(&m, Message_Statics::checkpoint_finishMark_Response);
  /* Mutator-threads send their NMT-trapped refs before replying */
  verbosePrint_checkpoint("Passed finish-mark checkpoint...");
}

void GC_Thread_Class::finalizeInternalDataStructuresForMark(){
  finalizeMarkStack();
}

inline void GC_Thread_Class::updateNMTbitIfNeeded(Oop* p, Oop &oldValue){
  if(!oldValue.is_mem()) fatal("Non-mem oop has no NMT bits");
  if(oldValue.getNMT() != Logical_Core::my_NMT()){
    Oop newValue = Oop::from_bits(oldValue.bits());
    newValue.setNMT( Logical_Core::my_NMT() );
    Oop::atomic_compare_and_swap(p, oldValue, newValue);
    oldValue.setNMT(Logical_Core::my_NMT());
  }
}

void GC_Thread_Class::updateNMTbitIfNeededAndPushOnStack(Oop* p){
  Oop oldValue = *p;
  updateNMTbitIfNeeded(p, oldValue); 
  assert(oldValue.getNMT() == Logical_Core::my_NMT());
  mark_stack_.push(oldValue.as_object());
}

/* Returns true if the traverse is completed. */
bool GC_Thread_Class::mark_traverse(Object* object){
  int page = comp_pageof(object);
  
  //Skip objects in pages alloacted after the start of the GC-cycle.
  if( ! m_pageLiveness[page].isAllocated() ) {
    //printf("Found object in non-Allocated-page %d\n",page);
    return false;
  }
  
  assert( !object->is_marked());
  if(object->is_free()) fatal();
  object->mark_without_store_barrier();
  

  // Mark the references it contains with the correct NMT value and add them to the worklist.
  FOR_EACH_STRONG_OOP_IN_OBJECT_EXCEPT_CLASS_RECORDING_WEAK_ROOTS(object, oopp){
    if( oopp->is_mem()){
      updateNMTbitIfNeededAndPushOnStack(oopp);
    }
  }
  if (object->contains_class_and_type_word()) {
    Oop class_oop = object->get_class_oop();
    object->setNMTbitOfClassOopIfDifferent( Logical_Core::my_NMT() );
    Object* o = class_oop.as_object_noLVB();
    assert(o != NULL);
    o->is_marked();
    mark_stack_.push( o );
  }
  if (Extra_Preheader_Word_Experiment){
    Oop* ephw_oop = (Oop*)(object->extra_preheader_word());
    if( ephw_oop->is_mem() ) {
      updateNMTbitIfNeededAndPushOnStack(ephw_oop);
    }
  }
  
  /**/
  
  
  
  // Add the size of this object to the liveness array.
  int size = comp_sizeof(object);

  
  if( size > page_size ){
    /*
     * Large object:
     * assume that pages are consecutive.
     * set all used-pages' liveness to page_size.
     */
    while(size>0){
      m_pageLiveness[page].addLiveBytes(page_size);
      page++;
      size-=page_size;
      assert( m_pageLiveness[page].isAllocated() );
    }
  } else {
    
    // Small object: easy
    int liveBytes = m_pageLiveness[page].liveBytes;
    m_pageLiveness[page].addLiveBytes(size);
    int newLiveBytes = m_pageLiveness[page].liveBytes;
    if(newLiveBytes<  liveBytes+size){
      assert(liveBytes == page_size);
      assert( newLiveBytes == page_size);
    }
    assert( m_pageLiveness[page].isAllocated() );
  }
  
  assert(object->is_marked());
  return true;
}

/***************************************************
 *                       RELOCATE                  *
 ***************************************************/

void GC_Thread_Class::phase_relocate(){
  phase->set_phase_relocate();
  assert( is_relocate_phase() );
  
  printf("GC\tstart relocate...\n");
  
  initInternalDataStructuresForRelocate();
  printf("GC\tstart protection...\n");
  //protectAlmostDeadPages();
  printf("GC\tall protected...\n");
  printf("GC\tstart relocate cp...\n");
  checkpoint_startRelocate();
  printf("GC\trelocate cp done...\n");
  printf("GC\tstart relocation itself...\n");
  doRelocation();
  
  printf("GC\tend relocate...\n");
  
  
  phase->unset_phase_relocate();
  assert( ! is_relocate_phase() );
}

void GC_Thread_Class::initInternalDataStructuresForRelocate(){
  // init forwarding table is not necessary since atm we store forward pointer in Objects. 
}

void protectPage(Page* page){
  mprotect(page, page_size, PROT_NONE);
}

void GC_Thread_Class::protectAlmostDeadPages(){
  FOR_EACH_PAGE(page){
    if(isAlmostDead(page->pageNumber())) {
      protectPage(page);
    }
  }
}

void GC_Thread_Class::checkpoint_startRelocate(){
  ScrubExistingStaleRefs_Oop_Closure oc;
  The_Squeak_Interpreter()->do_all_roots(&oc);
  
  checkpoint_startRelocate_Message_class m;
  doCheckpoint(&m,Message_Statics::checkpoint_startRelocate_Response);
  verbosePrint_checkpoint("Passed start-relocate checkpoint...");
}

void GC_Thread_Class::doRelocation(){
  /* traverse liveness array */
  FOR_EACH_PAGE(page){
    if( isAlmostDead(page->pageNumber()) ) {
      relocateLiveObjectsOfPage(page);
    }
  }
}

void GC_Thread_Class::relocateLiveObjectsOfPage(Page* page){
  // DO NOT!!! UNMARK relocated objects!
  // We will use the marked-bit to traverse the 'live'-objects in the remap-phase.
  int pageNbr = page->pageNumber();
  
  printf("relocateLiveObjectsOfPage %d\n",pageNbr);
  
  Page* unprotectedPage = (Page*)((char*)page + The_Memory_System()->unprotected_heap_offset);
  
  FOR_EACH_OBJECT_IN_UNPROTECTED_PAGE(unprotectedPage, object){
    if( !object->isFreeObject() && object->is_marked() ){
      object->relocateIfNoForwardingPointer( The_Memory_System()->my_heap() );
      //printf("Moved object to %p in the heap of core %d\n",object->getForwardingPointer(), Logical_Core::my_rank());
      m_pageLiveness[pageNbr].liveBytes  -=comp_sizeof(object);
    }/* else {
      unmarked or free object objects are dead and do not need further treatment.
      }*/
  }
}

/***************************************************
 *                        REMAP                    *
 ***************************************************/

void GC_Thread_Class::phase_remap(){
  phase->set_phase_remap();
  assert( is_remap_phase() );
  
  initMarkStack();
  checkpoint_startRemap();
  remapAndUnMarkAllFromMarkStack();
  freeFreePages();
  finalizeMarkStack();
  
  phase->unset_phase_remap();
  assert( ! is_remap_phase() );
}

void GC_Thread_Class::checkpoint_startRemap(){
  verbosePrint_checkpoint("Start checkpoint_startRemap...");
  checkpoint_startRemap_Message_class m;
  doCheckpoint(&m,Message_Statics::checkpoint_startRemap_Response);
  verbosePrint_checkpoint("Passed checkpoint_startRemap...");
}

void GC_Thread_Class::remapAndUnMarkAllFromMarkStack(){
  int popCount    = 0;
  int unmarkCount = 0;
  
  while( !mark_stack_.is_empty()){
    
    Object* object = mark_stack_.pop(); popCount++;
    
    if (object == NULL) fatal("Popped objects should never be NULL...");
    
    if( object->is_marked() ){
      
      assert( object->is_marked());
      if(object->is_free()) fatal();
      object->unmark_without_store_barrier(); unmarkCount++;
      
      // Strong oops
      FOR_EACH_STRONG_OOP_IN_OBJECT_EXCEPT_CLASS_RECORDING_WEAK_ROOTS(object, oopp) {
        if(oopp->is_mem()){
          Object* o = oopp->as_object();
          mark_stack_.push( o );
        }
      }
      
      // Class oop
      if (object->contains_class_and_type_word()) {
        Oop c = object->get_class_oop();
        mark_stack_.push(c.as_object());
      }
      
      // Extra preaheader
      if (Extra_Preheader_Word_Experiment){
        Oop* ephw_oop = (Oop*)(object->extra_preheader_word());
        if( ephw_oop->is_mem() ) {
          mark_stack_.push(ephw_oop->as_object());
        }
      }
    }
  }
  printf("Remapping completed unmarked=%d, pops=%d\n",unmarkCount,popCount);
  
  int marked = 0;
  int oc = 0;
  FOR_EACH_OBJECT(obj){
    if( !obj->is_free() ){
      oc++;
      if(obj->is_marked() ){
        marked++;
      }
    }
  }
  printf("Still marked objects:%d (%d)\n",marked,oc);
  
}

void GC_Thread_Class::freeFreePages(){
  /* traverse liveness array */
  FOR_EACH_PAGE(page){
    if( m_pageLiveness[page->pageNumber()].liveBytes < LIVENESS_THRESHOLD ) freeFreePage(page);
  }
}

void GC_Thread_Class::freeFreePage(Page* page){
  mprotect(page,page_size, PROT_READ | PROT_WRITE | PROT_EXEC);
  The_Memory_System()->free(page);
}

/***************************************************
 *                        OTHER                    *
 ***************************************************/

void GC_Thread_Class::verbosePrint_checkpoint(const char* str){
  if(VERBOSE_CHECKPOINTS)printf("%s\n",str);
}

void GC_Thread_Class::doCheckpoint(checkpointMessage_class* m, Message_Statics::messages responseType){
  FOR_ALL_OTHER_RANKS(i) {
    m->send_to(i);
  }
  Message_Statics::receive_and_handle_all_checkpoint_responses_GC(responseType);
}

void GC_Thread_Class::checkpoint_simple(){
  verbosePrint_checkpoint("Start simple checkpoint...");
  checkpointMessage_class m;
  doCheckpoint(&m, Message_Statics::checkpointResponse);
  verbosePrint_checkpoint("Passed simple checkpoint...");
}

int GC_Thread_Class::comp_sizeof(Object* object){ return object->total_byte_size(); }

int GC_Thread_Class::comp_pageof(Object* object){ return object->my_pageNumber(); }

void GC_Thread_Class::initMarkStack(){
  printf("INIT MARK_STACK\n");
  mark_stack_ = GC_Oop_Stack();
  assert_always( mark_stack_.is_empty() );
}

void GC_Thread_Class::finalizeMarkStack(){
  printf("FINALIZE MARK_STACK\n");
  assert_always( mark_stack_.is_empty() );
}

void GC_Thread_Class::addToMarkStack(Oop* p){
  assert(Logical_Core::running_on_GC()); // Needs to be on GC_thread to be thread safe.
  mark_stack_.push(p->as_object());
}

Oop GC_Thread_Class::lookUpNewLocation(Oop p){
  Object* o = p.as_object_in_unprotected_space();
  if(o->isForwardingPointer()){
    return o->getForwardingPointer();
  } else {
    return Oop::from_bits(0);
  }
}


void GC_Thread_Class::setAsAwaitingFinishedGCCycle(int rank){
  awaitingFinished[rank] = true;
}

void GC_Thread_Class::printAwaitingArray(){
  /*
   * Example:
   * 0    1   2   3
   * T    F   F   T
   */
  printf("GC-awaiting cores:\n");
  for(int i=0; i<Logical_Core::group_size ; i++){
    printf("%d\t",i);
  }
  printf("\n");
  for(int i=0; i<Logical_Core::group_size ; i++){
    printf("%s\t",awaitingFinished[i]?"T":"F");
  }
  printf("\n");
}

bool GC_Thread_Class::add_weakRoot(Oop x) {
  if (weakRootCount >= (sizeof(weakRoots) / sizeof(weakRoots[0]))) return false;
  
  weakRoots[weakRootCount++] = x;
  return true;
}

void GC_Thread_Class::finalize_weak_arrays() {
  for (u_int32 i = 0;  i < weakRootCount; ++i)
    finalizeReference(weakRoots[i].as_object());
  weakRootCount = 0;
}

void GC_Thread_Class::finalizeReference(Object_p weak_obj) {
  int nonWeakCnt = weak_obj->nonWeakFieldsOf();
  FOR_EACH_WEAK_OOP_IN_OBJECT(weak_obj, oop_ptr) {
    Oop x = *oop_ptr;
    if (    x.is_mem()
        &&  x != m_initialInterpreter->roots.nilObj
        &&  has_been_or_will_be_freed_by_this_ongoing_gc(x)) {
      *oop_ptr = m_initialInterpreter->roots.nilObj; // no store checks, no coherence operations, in the midst of GC
      if (nonWeakCnt >= 2) weak_obj->weakFinalizerCheckOf();
      //signalFinalization(x) on theInterperter (old version)
    }
  }
}

bool GC_Thread_Class::has_been_or_will_be_freed_by_this_ongoing_gc(Oop x) {
  return x.is_mem()
  &&  x != m_initialInterpreter->roots.nilObj
  &&  ( !The_Memory_System()->contains(x.as_object()) ||  !x.as_object()->is_marked());
}

void GC_Thread_Class::setInitialInterpreter( Squeak_Interpreter* initialInterpreter ){
  m_initialInterpreter = new Squeak_Interpreter();
  m_initialInterpreter->roots = initialInterpreter->roots;
}

void GC_Thread_Class::printLivenessArray( LPage* la ){
  printf("printLivenessArray\n");
  printf("------------------\n");
  for(int i=0 ; i < The_Memory_System()->calculate_total_pages( page_size ) ; i++){
    if(la[i].isAllocated()){
      printf("Page%d\t%6d\t%.2f\n",i, la[i].liveBytes, (float)la[i].liveBytes/(float)page_size);
    }
  }
  printf("------------------\n");
}

bool GC_Thread_Class::isAlmostDead(int pageNbr){
  if( ! (is_relocate_phase() || is_remap_phase())) {
    printf("Illegal phase for isAlmostDead: %d\n", phase->phase);
  }
  //assert( is_relocate_phase() || is_remap_phase() );
  return ( m_pageLiveness[pageNbr].liveBytes < LIVENESS_THRESHOLD );
}


