//
//  parallel_gc_thread.h
//  RoarVM
//
//  Created by Mattias De Wael on 27/07/11.
//  Copyright 2011 VUB. All rights reserved.
//

class GC_State{
public:
  bool is_mark_phase(){ return is_phase(mark_phase_tag); };
  bool is_relocate_phase(){ return is_phase(relocate_phase_tag); };
  bool is_remap_phase(){ return is_phase(remap_phase_tag); };
  
  
  void set_phase_mark()  { set_phase(mark_phase_tag);   };
  void unset_phase_mark(){ unset_phase(mark_phase_tag); };
  
  void set_phase_relocate()  { set_phase(relocate_phase_tag);   };
  void unset_phase_relocate(){ unset_phase(relocate_phase_tag); };
  
  void set_phase_remap()  { set_phase(remap_phase_tag);   };
  void unset_phase_remap(){ unset_phase(remap_phase_tag); };
  int phase;
  
private:
  static const int mark_phase_tag     = 1;
  static const int relocate_phase_tag = 2;
  static const int remap_phase_tag    = 4;
  
  /* Inspect current running GC-phase(s) */
  bool is_phase(int phase_tag){ return (phase & phase_tag); };
  
  
  void set_phase(int phase_tag){ phase |= phase_tag; };
  void unset_phase(int phase_tag){ phase &= ~phase_tag; };
  
  
};

class GC_Thread_Class
{
public:
  static int const LIVENESS_THRESHOLD = (Mega/3);
  
  void addRoots( GC_Oop_Stack* aRootStack );
  void addToMarkStack(Oop* object);
  void setAsAwaitingFinishedGCCycle(int rank);
  bool isAlmostDead(Page* p);
  
  /* Inspect current running GC-phase(s) */
  bool is_mark_phase(){ return phase->is_mark_phase(); };
  bool is_relocate_phase(){ return phase->is_relocate_phase(); };
  bool is_remap_phase(){ return phase->is_remap_phase(); };
  
  GC_Thread_Class()
  : m_stoprequested(false), m_running(false),m_rank(Logical_Core::num_cores+1),phase(new GC_State())
  {
    mark_stack_ = GC_Oop_Stack();
    awaitingFinished = (bool*)(malloc(sizeof(bool)*Logical_Core::group_size));
    for(int i=0; i<Logical_Core::group_size ; i++){
      awaitingFinished[i] = false;
    }
  }
  
  ~GC_Thread_Class()
  {
  }
  
  void start()
  {
    assert(m_running == false);
    m_running = true;
    pthread_create(&m_myThread, NULL, &GC_Thread_Class::start_gc_thread, this);
  }
  
  void stop()
  {
    assert(m_running == true);
    m_running = false;
    m_stoprequested = true;
    pthread_join(m_myThread, NULL);
  }
  
  Oop* lookUpNewLocation(Oop p);
  void addNewTopContents(Contents* c){
    mark_stack_.addNewTopContents( c );
  }
  
  bool add_weakRoot(Oop);
  
  void setNilObj(Oop nilObj );
  
private:
  bool* awaitingFinished;
  
  u_int32   weakRootCount;
  Oop       weakRoots[10000];
  
  volatile bool m_stoprequested;
  volatile bool m_running;
  
  GC_State* phase;
  
  LPage* m_pageLiveness;
  //GC_Oop_Stack* mark_stack;
  GC_Oop_Stack mark_stack_;
  int m_rank;
  Oop m_nilObj;
  
  pthread_t m_myThread;
  
  
  static void* start_gc_thread(void *obj)
  {
    //All we do here is call the do_work() function
    reinterpret_cast<GC_Thread_Class *>(obj)->do_work();
    return NULL;
  }
  
  
private:
  
  
  void do_work();
  
  // MARK
  void phase_mark();
  
  void initInternalDataStructuresForMark();
  bool flipNMT(){ Logical_Core::my_core()->setNMT( ! Logical_Core::my_NMT() ); }
  inline void updateNMTbitIfNeeded(Oop* p, Oop &oldValue);
  void updateNMTbitIfNeededAndPushOnStack(Oop* p);
  void checkpoint_startMark();
  void checkpoint_finishMark();
  void finalizeInternalDataStructuresForMark();
  void mark_traverse(Object*);
  
  // RELOCATE
  void phase_relocate();
  void initInternalDataStructuresForRelocate();
  void protectAlmostDeadPages();
  void doRelocation();
  void relocateLiveObjectsOfPage(Page* page);
  
  // REMAP
  void phase_remap();
  void checkpoint_startRemap();
  void remapAndUnMarkAllFromMarkStack();
  void freeFreePages();
  void freeFreePage(Page* page);
  
  // OTHER
  int  comp_sizeof(Object*);
  int  comp_pageof(Object*);
  
  inline void verbosePrint_checkpoint(const char* str);
  void printLivenessArray( LPage* la );
  
  void doCheckpoint(checkpointMessage_class* m, Message_Statics::messages responseType);
  void checkpoint_simple();
  void initMarkStack();
  void finalizeMarkStack();
  
  void printAwaitingArray();
  
  void finalize_weak_arrays();
  void finalizeReference(Object_p weak_obj);
  void checkpoint_startRelocate();
  bool has_been_or_will_be_freed_by_this_ongoing_gc(Oop x);
};

class SetNMTbitAndCollectRoots_Oop_Closure: public Oop_Closure {
  
private:
public:
  SetNMTbitAndCollectRoots_Oop_Closure(int NMT) : Oop_Closure() {
    roots = new GC_Oop_Stack();
    m_NMT = NMT;
  }
  int m_NMT;
  GC_Oop_Stack* roots;
  
  
  
  void value(Oop* p, Object_p) {
    bool succeeded = false;
    if (!p->is_mem()) return;
    do{
      Oop newOop = Oop::from_bits(p->bits());
      newOop.setNMT(m_NMT);
      succeeded = Oop::atomic_compare_and_swap(p,*p,newOop); // Actually this should be already thread-safe (overkill?)
    } while(!succeeded);
    Object* theObject = p->as_object();
    roots->push( theObject );
  }
  
  virtual const char* class_name(char*) { return "SetNMTbitAndCollectRoots_Oop_Closure"; }
  
};

class ScrubExistingStaleRefs_Oop_Closure: public Oop_Closure {
  
public:
  ScrubExistingStaleRefs_Oop_Closure() : Oop_Closure() {
  }
  
  void value(Oop* p, Object_p) { 
    //Scrub by dereferencing.
    if(p->is_mem()) p->as_object();
    else { /* do integers need to be moved while scrubbing */ }
  }
  
  virtual const char* class_name(char*) { return "ScrubExistingStaleRefs_Oop_Closure"; }
  
  
};

class CollectRoots_Oop_Closure: public Oop_Closure {
  
private:
public:
  CollectRoots_Oop_Closure() : Oop_Closure() {
    roots = new GC_Oop_Stack();
  }
  int m_NMT;
  GC_Oop_Stack* roots;
  
  
  
  void value(Oop* p, Object_p) {
    bool succeeded = false;
    if (!p->is_mem()) return;
    
    //Object_p o_ptr = The_Squeak_Interpreter()->newMethod_obj();
    Oop p1 = Oop::from_bits(p->raw_bits());
    roots->push( p->as_object() );
    Oop p2 = Oop::from_bits(p->raw_bits());
    if( p1.raw_bits() != p2.raw_bits()){
      printf("Found moved root:from %d to %d\n",p1.bits(),p2.bits());
    }
  }
  
  virtual const char* class_name(char*) { return "CollectRoots_Oop_Closure"; }
  
};