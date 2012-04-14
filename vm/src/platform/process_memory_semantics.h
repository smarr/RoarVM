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


# if Using_Processes

class Process_Memory_Semantics : public Abstract_Memory_Semantics {
public:
  static Logical_Core* _my_core;
  static int           _my_rank;
  static u_int64       _my_rank_mask;

public:
  static const size_t max_num_threads_on_threads_or_1_on_processes = 1;
  static inline size_t rank_on_threads_or_zero_on_processes() { return 0; }

  static inline void initialize_memory_system()       {}
  static inline void initialize_local_memory_system() {}
  static inline void initialize_interpreter()         {}
  static inline void initialize_local_interpreter()   {}
  static inline void initialize_timeout_timer()       {}
  static inline void initialize_local_timeout_timer() {}


  static inline bool cores_are_initialized() { return true; }
  static inline Logical_Core* my_core() { return _my_core; }
  static inline int           my_rank() { return _my_rank; }
  static inline u_int64       my_rank_mask() { return _my_rank_mask; }
  
  static inline void initialize_logical_core() {}
  static inline void initialize_local_logical_core() {}
  
  static void go_parallel(void (*helper_core_main)(), char* argv[]) {
    OS_Interface::start_processes(helper_core_main, argv);
  }
  
  static inline int get_group_rank() { return OS_Interface::get_process_rank(); }
  
  static inline void* shared_malloc(u_int32 sz) {
    return OS_Interface::rvm_malloc_shared(sz);
  }
  static inline void* shared_calloc(u_int32 num_members, u_int32 mem_size)  {
    return OS_Interface::rvm_calloc_shared(num_members, mem_size);
  }

  static inline void shared_free(void* ptr) {
    OS_Interface::rvm_free_shared(ptr);
  }  

  static inline bool is_using_threads() { return false; }
};

class  Memory_System;
extern Memory_System _memory_system;

//#define The_Memory_System() (&_memory_system)
// At least the Tilera compiler does not like the inlines, costs about 2-5% performance
inline  FORCE_INLINE  Memory_System* The_Memory_System() { return &_memory_system;  };


extern Squeak_Interpreter _interpreter;

//#define The_Squeak_Interpreter() (&_interpreter)
// At least the Tilera compiler does not like the inlines, costs about 2-5% performance
inline  FORCE_INLINE Squeak_Interpreter* The_Squeak_Interpreter() { return &_interpreter;  }


class  Timeout_Timer_List_Head;
extern Timeout_Timer_List_Head _timeout_head;
inline FORCE_INLINE Timeout_Timer_List_Head* The_Timeout_Timer_List_Head() { return &_timeout_head; }


# endif

