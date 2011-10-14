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


pthread_t     POSIX_OS_Interface::threads[Max_Number_Of_Cores];
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



int POSIX_OS_Interface::abort_if_error(const char* msg, int err) {
  if (err == 0)  return err;
  lprintf( "%s failed, error: %d\n", msg, err);
  abort();
  return 0;
}


