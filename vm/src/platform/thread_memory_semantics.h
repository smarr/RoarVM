/******************************************************************************
 *  Copyright (c) 2008 - 2010 IBM Corporation and others.
 *  All rights reserved. This program and the accompanying materials
 *  are made available under the terms of the Eclipse Public License v1.0
 *  which accompanies this distribution, and is available at
 *  http://www.eclipse.org/legal/epl-v10.html
 * 
 *  Contributors:
 *    David Ungar, IBM Research - Initial Implementation
 *    Sam Adams, IBM Research - Initial Implementation
 *    Stefan Marr, Vrije Universiteit Brussel - Port to x86 Multi-Core Systems
 ******************************************************************************/


# if Using_Threads

class Timeout_Timer_List_Head;

class Thread_Memory_Semantics : public Abstract_Memory_Semantics {

#pragma mark Memory System
  
// It is not necessary to replicate the memory system in shared-memory
// environments, this is the standard case for the RVM.
// However, to have a version with equal functionallity, this flag can be
// used to enforce replication.
# if  Replicate_PThread_Memory_System
private:
  static void _dtor_memory_system_key(void* local_obj);
public:
  # if Use_ThreadLocals
    static __thread Memory_System* memory_system;
  # else
    static pthread_key_t memory_system_key;
  # endif // !Use_ThreadLocals
# endif
  
public:
  static void initialize_memory_system();
  static void initialize_local_memory_system();


#pragma mark Interpreter
  
# if !Force_Direct_Squeak_Interpreter_Access
  // For test it can be enforced to use the same strategy as for processes
  // to allocate the interpreter, but that is usually not done.
  // Furthermore, if Force_Direct_Squeak_Interpreter_Access would be set,
  // the RVM could only use a single core.
  # if Use_ThreadLocals
  public:
    static __thread Squeak_Interpreter* interpreter;
  # else
  private:
    static void _dtor_interpreter(void* interpreter);
  public:
    static pthread_key_t interpreter_key;
  # endif // !Use_ThreadLocals
# endif
  
  static void initialize_interpreter();
  static void initialize_local_interpreter();
  
#pragma mark Timeout Timers
  
# if Force_Direct_Timeout_Timer_List_Head_Access
private:
  static Timeout_Timer_List_Head _head;
public:
  static void initialize_timeout_timer()       {}
  static void initialize_local_timeout_timer() {}
  
# else
  # if Use_ThreadLocals
    static __thread Timeout_Timer_List_Head* timeout_head;
  # else
  private:
    static void _dtor_timeout(void* local_head);
  public:
    static pthread_key_t timeout_key;
  # endif // !Use_ThreadLocals
  static void initialize_timeout_timer();
  static void initialize_local_timeout_timer();
# endif


#pragma mark Miscellaneous
  
private:
# if Use_ThreadLocals
  static __thread Logical_Core* _my_core;
# else
  static pthread_key_t my_core_key;
  
  static void _dtor_my_core_key(void*) {
    pthread_setspecific(my_core_key, NULL);
  }
# endif // if !Use_ThreadLocals

public:
# if Use_ThreadLocals
  static inline bool cores_are_initialized() { return _my_core != NULL; }
# else
  static inline bool cores_are_initialized() { return my_core_key != 0; }
# endif
  
  static const size_t max_num_threads_on_threads_or_1_on_processes = Max_Number_Of_Cores;
  
  static Logical_Core* my_core();
  static int           my_rank();
  static u_int64       my_rank_mask();
  static inline size_t rank_on_threads_or_zero_on_processes() { return my_rank(); } 

  static void initialize_logical_cores();
  static void initialize_local_logical_core();
  static void initialize_local_logical_core(int rank);

  
  static void go_parallel(void (*helper_core_main)(), char* argv[]) { 
    OS_Interface::start_threads(helper_core_main, argv);
  }
  
  static inline int get_group_rank() { return OS_Interface::get_thread_rank(); }
  
  static inline void* shared_malloc(u_int32 sz) {
    return malloc(sz);
  }
  static inline void* shared_calloc(u_int32 num_members, u_int32 mem_size)  {
    return calloc(num_members, mem_size);
  }
  static inline void  shared_free(void* ptr) {
    free(ptr);
  }
      
  static inline bool is_using_threads() { return true; }
};

#pragma mark -
#pragma mark Global Accessor Functions

class Memory_System;
# if  !Replicate_PThread_Memory_System
  extern Memory_System _memory_system;

  inline FORCE_INLINE Memory_System* The_Memory_System() {
    return &_memory_system;
  }
# else
  # if Use_ThreadLocals
    inline FORCE_INLINE Memory_System* The_Memory_System() {
      assert(Memory_Semantics::memory_system != NULL /* ensure it is initialized */);
      return Memory_Semantics::memory_system;
    }
  # else
    inline FORCE_INLINE Memory_System* The_Memory_System() {
      assert(Memory_Semantics::memory_system_key != 0 /* ensure it is initialized */);
      return (Memory_System*)pthread_getspecific(Memory_Semantics::memory_system_key);
    }
  # endif // !Use_ThreadLocals
# endif



# if Force_Direct_Squeak_Interpreter_Access
  extern Squeak_Interpreter _interpreter;

  //#define The_Squeak_Interpreter() (&_interpreter)
  // At least the Tilera compiler does not like the inlines, costs about 2-5% performance
  inline FORCE_INLINE Squeak_Interpreter* The_Squeak_Interpreter() { return &_interpreter;  }
# else
  # if Use_ThreadLocals
    inline FORCE_INLINE Squeak_Interpreter* The_Squeak_Interpreter() {
      assert(Memory_Semantics::interpreter != NULL /* ensure it is initialized */);
      return Memory_Semantics::interpreter;
    }
  # else
    inline FORCE_INLINE Squeak_Interpreter* The_Squeak_Interpreter() {
      assert(Memory_Semantics::interpreter_key != 0 /* ensure it is initialized */);
      return (Squeak_Interpreter*)pthread_getspecific(Memory_Semantics::interpreter_key);
    }
  # endif
# endif

class Timeout_Timer_List_Head;
# if Force_Direct_Timeout_Timer_List_Head_Access
  extern Timeout_Timer_List_Head _timeout_head;

  inline FORCE_INLINE Timeout_Timer_List_Head* The_Timeout_Timer_List_Head() {
    return &_head;
  }
# else
  # if Use_ThreadLocals
    inline FORCE_INLINE Timeout_Timer_List_Head* The_Timeout_Timer_List_Head() {
      return Memory_Semantics::timeout_head;
    }
  # else
    inline FORCE_INLINE Timeout_Timer_List_Head* The_Timeout_Timer_List_Head() {
      return (Timeout_Timer_List_Head*)pthread_getspecific(Memory_Semantics::timeout_key);
    }
  # endif // !Use_ThreadLocals
# endif

# endif // Using_Threads

