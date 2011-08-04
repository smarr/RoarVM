//
//  parallel_gc_thread.h
//  RoarVM
//
//  Created by Mattias De Wael on 27/07/11.
//  Copyright 2011 VUB. All rights reserved.
//

#define NUMBER_OF_PAGES_IN_THE_HEAP 1000 //TODO
#define LIVENESS_THRESHOLD 1000 //TODO


class GC_Thread_Class
{
public:
    static void addToMarkStack(Oop* p){ instance->_addToMarkStack(p); }
    static void setAsAwaitingFinishedGCCycle(int rank){
        instance->_setAsAwaitingFinishedGCCycle(rank);
    }
    
    static GC_Thread_Class* getInstance(){ return instance; }
    
    static void addFilledContentsToMarkStack(Contents* contents){
            GC_Oop_Stack* gc_markStack = instance->getMarkStack();
            bool CAS_succeeded = false;
            do {
                Contents* contentsOf_gc_markStack = gc_markStack->contents;
                CAS_succeeded = GC_Oop_Stack::atomic_compare_and_swap(&gc_markStack->contents,contentsOf_gc_markStack,contents);
                contents->next_contents = contentsOf_gc_markStack;
            } while (!CAS_succeeded);
    }
    
    GC_Oop_Stack* getMarkStack(){return mark_stack; }
    
    GC_Thread_Class(Logical_Core gc_core)
    : m_stoprequested(false), m_running(false),m_NMT(0),m_rank(Logical_Core::num_cores+1),m_core(gc_core)
    {
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
        assert(instance == NULL);
        instance = this;
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
    
private:
    static GC_Thread_Class* instance;
    bool* awaitingFinished;
    
    volatile bool m_stoprequested;
    volatile bool m_running;
    
    Logical_Core m_core;
    int m_NMT;
    int m_pageLiveness[NUMBER_OF_PAGES_IN_THE_HEAP];
    GC_Oop_Stack* mark_stack;
    int m_rank;
    Abstract_Object_Heap* m_objectHeap;
    
    pthread_t m_myThread;
    
    
    static void* start_gc_thread(void *obj)
    {
        //All we do here is call the do_work() function
        reinterpret_cast<GC_Thread_Class *>(obj)->do_work();
        return NULL;
    }
    
    void do_work()
    {
        Thread_Memory_Semantics::initialize_corekey_for_GC();
        printf("GC_thread running (%d)...\n", Logical_Core::my_rank());
        
      
      //checkpoint_simple();
      
      
        //fatal("Init m_ObjectHeap");
        /*
        printf("start stack test:\n");
        GC_Oop_Stack::tests();
        printf("stack test OK\n");
         */
      TEST_force_protectedPage_signal_trap();
        while (!m_stoprequested)
        {
            /* Do no real work yet...
             phase_mark();
             phase_relocate();
             phase_remap();
             */
            m_stoprequested = true;
        }
    }
    
    // MARK
    void phase_mark();
    void initInternalDataStructuresForMark();
    int  flipNMT(){ return (m_NMT = (!m_NMT)); }
    inline void updateNMTbitIfNeeded(Oop* p);
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
    void relocateLiveObjectsOfPage(int pageNbr);
    
    // REMAP
    void phase_remap();
    void checkpoint_startRemap();
    void remapAndUnMarkAllFromMarkStack();
    void freeFreePages();
    void freeFreePage(int pageNbr);
    
    // OTHER
    int  comp_sizeof(Object*);
    int  comp_pageof(Object*);
    
    void doCheckpoint(checkpointMessage_class* m, Message_Statics::messages responseType);
    void checkpoint_simple();
    void initMarkStack();
    void finalizeMarkStack();
    
    void _addToMarkStack(Oop* object);
    void _setAsAwaitingFinishedGCCycle(int rank);
    
    void printAwaitingArray();

    
    void checkpoint_startRelocate();
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
        do{
            Oop newOop = Oop::from_bits(p->bits());
            newOop.setNMT(m_NMT);
            succeeded = Oop::atomic_compare_and_swap(p,*p,newOop);
        } while(!succeeded);
        
        roots->push( p->as_object() );
    }
        
    virtual const char* class_name(char*) { return "MakeLinkedList_Oop_Closure"; }
    
};

class ScrubExistingStaleRefs_Oop_Closure: public Oop_Closure {

public:
ScrubExistingStaleRefs_Oop_Closure() : Oop_Closure() {
}

void value(Oop* p, Object_p) { 
    //Scrub
    p->as_object();
    fatal("NYI - Is this the correct way to derefence and trigger the protection trap?\n");
}
virtual const char* class_name(char*) { return "ScrubExistingStaleRefs_Oop_Closure"; }


};