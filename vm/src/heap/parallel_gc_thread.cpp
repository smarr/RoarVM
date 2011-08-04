//
//  Parallel_GC_Thread.cpp
//  RoarVM
//
//  Created by Mattias De Wael on 26/07/11.
//  Copyright 2011 VUB. All rights reserved.
//

#include <pthread.h>
#include "headers.h"

/***************************************************
 *                         MARK                    *
 ***************************************************/

void GC_Thread_Class::phase_mark(){
    initInternalDataStructuresForMark();
    
    flipNMT(); // Select a new value for NMT.
    
    
    checkpoint_startMark();
    
    do{
        while( !mark_stack->is_empty()){
            // All elements in the workset should be marked.
            // Pop one at the time and traverse.
            Object* object = mark_stack->pop();
            if(object != NULL && !object->is_marked() ) {
                mark_traverse(object);
            }
        }
        checkpoint_finishMark();  // Force all mutator-threads to push their current NMT-trapped-refs to the stack.
    }while(!mark_stack->is_empty()); //Restart if new objects are added.
    
    
}

void GC_Thread_Class::initInternalDataStructuresForMark(){
    initMarkStack();
    // Now intit all on zero, should be copy of array supplied by the MemManagement.
    for(int i=0 ; i<NUMBER_OF_PAGES_IN_THE_HEAP ; i++){ // Set all pages liveness to zero...
        m_pageLiveness[i]=0;
    }
}

void GC_Thread_Class::checkpoint_startMark(){
    checkpoint_startMark_Message_class m( m_NMT );
    doCheckpoint(&m, Message_Statics::checkpoint_startMark_Response);
    /* Mutator-threads mark their roots-set with new NMT and reply with the refs in their root set. */
    printf("Passed start-mark checkpoint...\n");
}

void GC_Thread_Class::checkpoint_finishMark(){
    checkpoint_finishMark_Message_class m;
    doCheckpoint(&m, Message_Statics::checkpoint_finishMark_Response);
    /* Mutator-threads send their NMT-trapped refs before replying */
    printf("Passed finish-mark checkpoint...\n");
}

void GC_Thread_Class::finalizeInternalDataStructuresForMark(){
    finalizeMarkStack();
}

inline void GC_Thread_Class::updateNMTbitIfNeeded(Oop* p){
    Oop oldValue = *p;
    if(oldValue.getNMT() != m_NMT){
        Oop newValue = Oop::from_bits(oldValue.bits());
        newValue.setNMT(m_NMT);
        //atomic_compare_and_swap(p, oldValue, newValue);
    }
}

void GC_Thread_Class::updateNMTbitIfNeededAndPushOnStack(Oop* p){
    updateNMTbitIfNeeded(p);
    mark_stack->push(p->as_object());
}

void GC_Thread_Class::mark_traverse(Object* object){
    if(object->is_marked()) return;
    if(object->is_free()) fatal();
    object->mark_without_store_barrier();
    
    /**/
    
    // Mark the references it contains with the correct NMT value and add them to the worklist.
    FOR_EACH_STRONG_OOP_IN_OBJECT_EXCEPT_CLASS_RECORDING_WEAK_ROOTS(object, oopp,NULL){
        if( oopp->is_mem()){
            updateNMTbitIfNeededAndPushOnStack(oopp);
        }
    }
    if (object->contains_class_and_type_word()) {
        Oop class_oop = object->get_class_oop();
        object->setNMTbitOfClassOopIfDifferent( m_NMT );
        mark_stack->push(class_oop.as_object());
    }
    if (Extra_Preheader_Word_Experiment){
        Oop* ephw_oop = (Oop*)(object->extra_preheader_word());
        updateNMTbitIfNeededAndPushOnStack(ephw_oop);
    }
    
    /**/
    
    
    
    // Add the size of this object to the liveness array.
    fatal("NYI - make this correct.");
    fatal("NYI - do special treatment for 'large' objects");
    int size = comp_sizeof(object);
    int page = comp_pageof(object);
    
    if( size > PAGE_SIZE ){
        /*
         * Large object:
         * assume that pages are consecutive.
         * set all used-pages' liveness to PAGE_SIZE.
         */
        while(size>0){
            m_pageLiveness[page]   = PAGE_SIZE;
            page++;
            size-=PAGE_SIZE;
        }
    } else {
        // Small object: easy
        m_pageLiveness[page]+=size;
    }
    
}

/***************************************************
 *                       RELOCATE                  *
 ***************************************************/

void GC_Thread_Class::phase_relocate(){
    initInternalDataStructuresForRelocate();
    protectAlmostDeadPages();
    checkpoint_startRelocate();
    doRelocation();
}

void GC_Thread_Class::initInternalDataStructuresForRelocate(){
    // init forwarding table...
    fatal("NYI - forwarding table not yet implemented");
}

void protectPage(int pageNumber){
    fatal("NYI - protect a page");
    void* page = NULL;
    mprotect(page, PAGE_SIZE, PROT_NONE);
}

void GC_Thread_Class::protectAlmostDeadPages(){
    for(int i=0 ; i<NUMBER_OF_PAGES_IN_THE_HEAP ; i++){
        if( m_pageLiveness[i] < LIVENESS_THRESHOLD ) protectPage(i);
    }
}

void GC_Thread_Class::checkpoint_startRelocate(){
    checkpoint_startRelocate_Message_class m;
    doCheckpoint(&m,Message_Statics::checkpoint_startRelocate_Response);
    printf("Passed start-relocate checkpoint...\n");
}

void GC_Thread_Class::doRelocation(){
    /* traverse liveness array */
    for(int i=0 ; i<NUMBER_OF_PAGES_IN_THE_HEAP ; i++){
        if( m_pageLiveness[i] < LIVENESS_THRESHOLD ) relocateLiveObjectsOfPage(i);
    }
}

void GC_Thread_Class::relocateLiveObjectsOfPage(int pageNbr){
  // DO NOT!!! UNMARK relocated objects!
  // We will use the marked-bit to traverse the 'live')objects in the remap-phase.
  
  //traverse live objects in this page...
  fatal("Get page pointer to double-mapped-GC-page.");
  fatal("page->get_first_object");
  Object* object = NULL; 
  while (m_pageLiveness[pageNbr]>0) {
    if( !object->isFreeObject() && object->is_marked() ){
      object->relocateIfNoForwardingPointer( m_objectHeap );
      m_pageLiveness[pageNbr]-=comp_sizeof(object);
    }/* else {
      unmarked or free object objects are dead and do not need further treatment.
    }*/
    object = m_objectHeap->next_object(object);
  }
}

/***************************************************
 *                        REMAP                    *
 ***************************************************/

void GC_Thread_Class::phase_remap(){
    initMarkStack();
    checkpoint_startRemap();
    remapAndUnMarkAllFromMarkStack();
    
    freeFreePages();
    
    finalizeMarkStack();
    
    
}

void GC_Thread_Class::checkpoint_startRemap(){
    checkpoint_startRemap_Message_class m;
    doCheckpoint(&m,Message_Statics::checkpoint_startRemap_Response);
    printf("Passed start-remap checkpoint...\n");
}

void GC_Thread_Class::remapAndUnMarkAllFromMarkStack(){
  fatal("NYI - is dereferencing done by using as_object on oop? Does this trigger protection-trap?");
  while( !mark_stack->is_empty()){
    Object* object = mark_stack->pop();
    if( object->is_marked()){
      // Strong oops
      FOR_EACH_STRONG_OOP_IN_OBJECT_EXCEPT_CLASS_RECORDING_WEAK_ROOTS(object, oopp, NULL) {
        if(oopp->is_mem()){
          mark_stack->push(oopp->as_object());
        }
      }
      
      // Class oop
      if (object->contains_class_and_type_word()) {
        Oop c = object->get_class_oop();
        mark_stack->push(c.as_object());
      }
      
      // Extra preaheader
      if (Extra_Preheader_Word_Experiment){
        mark_stack->push(((Oop*)(object->extra_preheader_word()))->as_object());
      }
      object->unmark_without_store_barrier();
    }
  }
}

void GC_Thread_Class::freeFreePages(){
    /* traverse liveness array */
    for(int i=0 ; i<NUMBER_OF_PAGES_IN_THE_HEAP ; i++){
        if( m_pageLiveness[i] < LIVENESS_THRESHOLD ) freeFreePage(i);
    }
}

void GC_Thread_Class::freeFreePage(int pageNbr){
    fatal("NYI");
    // make free and make unprotected
    void* page = NULL;
    mprotect(page,PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC);
    //memorySystem->addToFreeList(page);
}

/***************************************************
 *                        OTHER                    *
 ***************************************************/
GC_Thread_Class* GC_Thread_Class::instance = NULL;

void GC_Thread_Class::doCheckpoint(checkpointMessage_class* m, Message_Statics::messages responseType){
    FOR_ALL_OTHER_RANKS(i) {
        m->send_to(i);
    }
    Message_Statics::receive_and_handle_all_checkpoint_responses_GC(responseType);
    printAwaitingArray();
}

void GC_Thread_Class::checkpoint_simple(){
    checkpointMessage_class m;
    doCheckpoint(&m, Message_Statics::checkpointResponse);
    printf("Passed simple checkpoint...\n");
}

int GC_Thread_Class::comp_sizeof(Object* object){ return object->total_byte_size(); }

int GC_Thread_Class::comp_pageof(Object* object){/* TODO */}

void GC_Thread_Class::initMarkStack(){
    if(mark_stack != NULL) delete mark_stack;
    mark_stack = new GC_Oop_Stack();
}

void GC_Thread_Class::finalizeMarkStack(){
    if(mark_stack != NULL) delete mark_stack;
}

void GC_Thread_Class::_addToMarkStack(Oop* p){
    assert(Logical_Core::running_on_GC()); // Needs to be on GC_thread to be thread safe.
    mark_stack->push(p->as_object());
}

Oop* GC_Thread_Class::lookUpNewLocation(Oop p){
  fatal("NYI");
  return NULL;
}


void GC_Thread_Class::_setAsAwaitingFinishedGCCycle(int rank){
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


