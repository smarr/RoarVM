/******************************************************************************
 *  Copyright (c) 2008 - 2011 IBM Corporation and others.
 *  All rights reserved. This program and the accompanying materials
 *  are made available under the terms of the Eclipse Public License v1.0
 *  which accompanies this distribution, and is available at
 *  http://www.eclipse.org/legal/epl-v10.html
 * 
 *  Contributors:
 *    Mattias De Wael, Vrije Universiteit Brussel - Parallel Garbage Collection
 *    Wouter Amerijckx, Vrije Universiteit Brussel - Parallel Garbage Collection
 ******************************************************************************/


class GC_Thread_Class {
public:
  static int const LIVENESS_THRESHOLD = (Mega/3);
  
  void addRoots( GC_Oop_Stack* aRootStack );
  void addToMarkStack(Oop* object);
  void setAsAwaitingFinishedGCCycle(int rank);
  bool isAlmostDead(int pageNbr);
  bool isAllocated(int pageNbr); 
  bool isCompletelyDead(int pageNbr);
  
  /* Inspect current running GC-phase(s) */
  bool is_mark_phase(){ return phase->is_mark_phase(); };
  bool is_relocate_phase(){ return phase->is_relocate_phase(); };
  bool is_remap_phase(){ return phase->is_remap_phase(); };
  
  GC_Thread_Class()
  : m_stoprequested(false), m_running(false),m_rank(Logical_Core::num_cores+1),phase(new GC_State()),m_pageLiveness(NULL)
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
  
  Oop lookUpNewLocation(Oop p);
  void addNewTopContents(Contents* c){
    mark_stack_.addNewTopContents( c );
  }
  
  bool add_weakRoot(Oop);
  
  GC_State* phase;
  
  void setInitialInterpreter(Squeak_Interpreter* initialInterpreter );
  
private:
  bool* awaitingFinished;
  
  u_int32   weakRootCount;
  Oop       weakRoots[10000];
  
  volatile bool m_stoprequested;
  volatile bool m_running;
  
  
  
  LPage* m_pageLiveness;
  //GC_Oop_Stack* mark_stack;
  GC_Oop_Stack mark_stack_;
  int m_rank;
  Squeak_Interpreter* m_initialInterpreter;
  
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
  inline bool updateNMTbitIfNeeded(Oop* p, Oop &oldValue);
  void updateNMTbitIfNeededAndPushOnStack(Oop* p);
  void checkpoint_startMark();
  void checkpoint_finishMark();
  void finalizeInternalDataStructuresForMark();
  bool mark_traverse(Object*);
  
  // RELOCATE
  void phase_relocate();
  void initInternalDataStructuresForRelocate();
  void freeAndProtectPages();
  void doRelocation();
  void relocateLiveObjectsOfPage(Page* page);
  
  // REMAP
  void phase_remap();
  void checkpoint_startRemap();
  void remapAndMarkAllFromMarkStack();
  void freeFreePages();
  void freeFreePage(Page* page);
  void unmarkAllObjects();
  
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

