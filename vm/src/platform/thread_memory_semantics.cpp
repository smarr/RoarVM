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


# include "headers.h"


#pragma mark Memory System

# if !Replicate_PThread_Memory_System
  Memory_System _memory_system;
  void Thread_Memory_Semantics::initialize_memory_system() {}
  void Thread_Memory_Semantics::initialize_local_memory_system() {}
# else
  # if Use_ThreadLocals
    __thread Memory_System* Thread_Memory_Semantics::memory_system = NULL;

    void Thread_Memory_Semantics::initialize_memory_system() {}

    void Thread_Memory_Semantics::initialize_local_memory_system() {
      memory_system = new Memory_System();
    }
  # else
    pthread_key_t Thread_Memory_Semantics::memory_system_key = 0;

    void Thread_Memory_Semantics::initialize_memory_system() {
      pthread_key_create(&memory_system_key, _dtor_memory_system_key);
    }

    void Thread_Memory_Semantics::_dtor_memory_system_key(void* local_obj) {
      Memory_System* mem_sys = (Memory_System*)local_obj;
      delete mem_sys;
    }

    void Thread_Memory_Semantics::initialize_local_memory_system() {
      Memory_System* mem_sys = new Memory_System();
      pthread_setspecific(memory_system_key, mem_sys);
    }
  # endif  // !Use_ThreadLocals
# endif // Replicate_PThread_Memory_System

#pragma mark Interpreter

# if Force_Direct_Squeak_Interpreter_Access
  Squeak_Interpreter _interpreter;
  void Thread_Memory_Semantics::initialize_interpreter() { }
# else
  # if Use_ThreadLocals
    __thread Squeak_Interpreter* Thread_Memory_Semantics::interpreter = NULL;

    void Thread_Memory_Semantics::initialize_interpreter() {}

    void Thread_Memory_Semantics::initialize_local_interpreter() {
      interpreter = new Squeak_Interpreter();
    }

  # else
    pthread_key_t Thread_Memory_Semantics::interpreter_key;

    void Thread_Memory_Semantics::initialize_interpreter() {
      interpreter_key = 0;
      pthread_key_create(&interpreter_key, _dtor_interpreter);
    }

    void Thread_Memory_Semantics::_dtor_interpreter(void* local) {
      Squeak_Interpreter* interp = (Squeak_Interpreter*)local;
      delete interp;
    }

    void Thread_Memory_Semantics::initialize_local_interpreter() {
      Squeak_Interpreter* interp = new Squeak_Interpreter();
      
      assert(interpreter_key != 0 /* i.e. thread local storage is initialized */);
      
      pthread_setspecific(interpreter_key, interp);
    }
  # endif // !Use_ThreadLocals
# endif // Force_Direct_Squeak_Interpreter_Access


#pragma mark Timeout Timers

# if !Force_Direct_Timeout_Timer_List_Head_Access
  # if Use_ThreadLocals
    __thread Timeout_Timer_List_Head* Thread_Memory_Semantics::timeout_head = NULL;

    void Thread_Memory_Semantics::initialize_timeout_timer() {}

    void Thread_Memory_Semantics::initialize_local_timeout_timer() {
      timeout_head = new Timeout_Timer_List_Head();
    }
  # else
    pthread_key_t Thread_Memory_Semantics::timeout_key;

    void Thread_Memory_Semantics::initialize_timeout_timer() {
      timeout_key = 0;
      pthread_key_create(&timeout_key, _dtor_timeout);
    }

    void Thread_Memory_Semantics::_dtor_timeout(void* local_head) {
      Timeout_Timer_List_Head* head = (Timeout_Timer_List_Head*)local_head;
      delete head;
    }

    void Thread_Memory_Semantics::initialize_local_timeout_timer() {
      Timeout_Timer_List_Head* head = new Timeout_Timer_List_Head();
      pthread_setspecific(timeout_key, head);
    }
  # endif // !Use_ThreadLocals
# endif // !Force_Direct_Timeout_Timer_List_Head_Access


#pragma mark Miscellaneous

# if Use_ThreadLocals
  __thread Logical_Core* Thread_Memory_Semantics::_my_core = NULL;

  void Thread_Memory_Semantics::initialize_logical_cores() {
    initialize_local_logical_core();
  }

  Logical_Core* Thread_Memory_Semantics::my_core() {
    assert(_my_core != NULL);
    return _my_core;
  }
# else
  pthread_key_t Thread_Memory_Semantics::my_core_key = 0;

  void Thread_Memory_Semantics::initialize_logical_cores() {
    my_core_key = 0;
    pthread_key_create(&my_core_key, _dtor_my_core_key);
    assert_always(my_core_key != 0);
    initialize_local_logical_core();
  }

  Logical_Core* Thread_Memory_Semantics::my_core() {
    assert(my_core_key != 0);
    return (Logical_Core*)pthread_getspecific(my_core_key);
  }
# endif // !Use_ThreadLocals


void Thread_Memory_Semantics::initialize_local_logical_core() {
  initialize_local_logical_core(Memory_Semantics::get_group_rank());
}


void Thread_Memory_Semantics::initialize_local_logical_core(int rank) {
# if Use_ThreadLocals
  _my_core = &logical_cores[rank];
# else
  pthread_setspecific(my_core_key, &logical_cores[rank]);
# endif // !Use_ThreadLocals
  initialize_local_timeout_timer();
  if (!Logical_Core::running_on_main())
    initialize_local_interpreter(); // must precede argument processing
  initialize_local_memory_system();
}


int Thread_Memory_Semantics::my_rank() {
  assert(cores_are_initialized());
  return my_core()->rank();
}


u_int64 Thread_Memory_Semantics::my_rank_mask() {
  assert(cores_are_initialized());
  return my_core()->rank_mask();
}

