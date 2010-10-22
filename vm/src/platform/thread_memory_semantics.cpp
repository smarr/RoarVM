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

pthread_key_t Thread_Memory_Semantics::my_core_key = 0;


# if !Replicate_PThread_Memory_System
  Memory_System _memory_system;
  void Thread_Memory_Semantics::initialize_memory_system() {}
  void Thread_Memory_Semantics::initialize_local_memory_system() {}
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

#   endif

# if Force_Direct_Squeak_Interpreter_Access
  Squeak_Interpreter _interpreter;
  void Thread_Memory_Semantics::initialize_interpreter() { }
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
# endif // Force_Direct_Squeak_Interpreter_Access


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


void Thread_Memory_Semantics::initialize_local_logical_core() {
  initialize_local_logical_core(Memory_Semantics::get_group_rank());
}


void Thread_Memory_Semantics::initialize_local_logical_core(int rank) {
  pthread_setspecific(my_core_key, &logical_cores[rank]);
  Timeout_Timer::init_threadlocal();
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

char* Thread_Memory_Semantics::map_heap_memory(int total_size,
                                               int bytes_to_map,
                                               int page_size_used_in_heap_arg,
                                               void* where,
                                               int offset,
                                               int main_pid,
                                               int flags) {
  assert_always(Max_Number_Of_Cores >= Logical_Core::group_size);
  
  static char* base_address = NULL; // threadsafe
  
  assert(cores_are_initialized());
  
  if (Logical_Core::running_on_main() && !base_address)
    base_address = (char*)calloc(1, total_size);
  
  assert_message(base_address, "Initialization order is broken, it is expected, that this has already been executed on main.");
  
  return base_address + offset;
}



