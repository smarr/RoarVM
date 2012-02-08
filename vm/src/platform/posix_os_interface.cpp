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


#include "headers.h"


pthread_t     POSIX_OS_Interface::threads  [Max_Number_Of_Cores];
pid_t         POSIX_OS_Interface::processes[Max_Number_Of_Cores];

pthread_key_t POSIX_OS_Interface::rank_key = 0;

/**
 * This function makes sure that each rank of the process group is running on
 * a dedicated processing unit (PU) reported by the OS.
 * It does not mean a physical unit, but includes also logical units like hyper
 * threads.
 */
void POSIX_OS_Interface::pin_thread_to_core(int32_t rank) {
# if On_Intel_Linux
  // On Linux, it looks pretty easy:
  // http://www.linuxjournal.com/article/6799
  // http://www.ibm.com/developerworks/linux/library/l-affinity.html
  cpu_set_t affinity_mask;
  CPU_ZERO(&affinity_mask);
  CPU_SET(rank, &affinity_mask);
  
  #include <sys/syscall.h>
  pid_t tid = (pid_t) syscall (SYS_gettid);
  
  if (sched_setaffinity(tid, sizeof(affinity_mask), &affinity_mask) < 0) {
    perror("Failed to set affinity");
    abort();
  }
  sleep(0); // make sure the OS schedule has a chance to do as we told him
# endif
}


int32_t POSIX_OS_Interface::last_rank = 0;

void* POSIX_OS_Interface::pthread_thread_main(void* param) {
  void (*routine)() = (void (*)())param;

  int32_t my_rank = __sync_add_and_fetch(&last_rank, 1);

  pthread_setspecific(rank_key, (const void*)my_rank);

  OS_Interface::pin_thread_to_core(my_rank);
  
  routine();
  
  pthread_exit(NULL);
}


void POSIX_OS_Interface::create_threads(const size_t num_of_threads, void (*helper_core_main)()) { 
  for (size_t i = 1; i < num_of_threads; i++) {    
    int err = pthread_create(&threads[i], NULL, pthread_thread_main, (void*)helper_core_main);
    if (err < 0) {
      // error
      // TODO: do it properly
      perror("Failed to create a thread.");
    }
  }
}


void POSIX_OS_Interface::start_threads(void (*helper_core_main)(), char** /* argv[] */) {
  // first initialize the main core, always rank==0 for threads
  threads[0] = pthread_self();
  pthread_key_create(&rank_key, NULL);
  pthread_setspecific(rank_key, (const void*)0);
  
  Logical_Core::initialize_all_cores();
  
  Memory_Semantics::initialize_logical_cores();

  Memory_Semantics::initialize_interpreter();
  Memory_Semantics::initialize_local_interpreter();
  
  create_threads(Logical_Core::num_cores, helper_core_main);
  
  Logical_Core::group_size = Logical_Core::num_cores;
  
  if (Logical_Core::group_size > 1)
    if (Force_Direct_Squeak_Interpreter_Access | Force_Direct_Timeout_Timer_List_Head_Access | Omit_PThread_Locks)
      fatal("A flag is set for performance measurement that is incompatible with multiple pthreads");
  
  Logical_Core* me = Logical_Core::my_core();
  if (me->rank() != get_thread_rank()) {
    fatal("OS_Interface::start_threads: rank id is inconsistently initialized!");
  }
  
  fprintf(stdout, "spawned %d helpers\n", Logical_Core::group_size - 1);
  OS_Interface::pin_thread_to_core(0);
}

# if Using_Processes

void POSIX_OS_Interface::start_processes(void (*helper_core_main)(), char* argv[]) {
  // go parallel; one core returns; others run helper_core_main fn
  
# warning STEFAN: refactor, add a setter method for initializing those values.
  Logical_Core::group_size = POSIX_Processes::group_size();
  Logical_Core::remaining  = Logical_Core::group_size
                              - POSIX_Processes::active_group_members();

  Memory_Semantics::_my_rank = POSIX_Processes::process_rank();
  Memory_Semantics::_my_rank_mask = 1LL << u_int64(Memory_Semantics::_my_rank);
  
  if (Logical_Core::group_size == 1  &&  Logical_Core::group_size < Logical_Core::num_cores) {
    lprintf("Will ask for num_proc: %d\n", Logical_Core::num_cores);
    
    int err = POSIX_Processes::start_group(Logical_Core::num_cores, argv);
    abort_if_error("exec", err);
    die("impossible to reach this point. start_group does only return on failure.");
  }
  
  Logical_Core::initialize_all_cores();
  Memory_Semantics::_my_core = &logical_cores[Memory_Semantics::_my_rank];
  
  Memory_Semantics::initialize_interpreter();
  Memory_Semantics::initialize_local_interpreter();
  
# warning Do We need to setup channels here?
  
  if (Logical_Core::running_on_main()) {
    fprintf(stdout, "spawned %d helpers\n", Logical_Core::group_size - 1);
    return;
  }
  else {
    (*helper_core_main)();
    char buf[BUFSIZ];
    Logical_Core::my_print_string(buf, sizeof(buf));
    lprintf( "helper finsihed: %s\n", buf);
    rvm_exit();
  }  
}

# endif // Using_Processes


int POSIX_OS_Interface::abort_if_error(const char* msg, int err) {
  if (err == 0)  return err;
  lprintf( "%s failed, error: %d\n", msg, err);
  abort();
  return 0;
}

Interprocess_Allocator* POSIX_OS_Interface::shared_memory_allocator() {
  static Interprocess_Allocator* allocator = NULL;
  size_t pool_size = 5 * 1024 * 1024;
  if (!allocator) {
    initialize(); // Do implicit initialization of the whole module
    void* mem = Memory_Semantics::shared_allocation_pool(pool_size);
    allocator = new Interprocess_Allocator(mem, pool_size);
  }
  return allocator;
}


