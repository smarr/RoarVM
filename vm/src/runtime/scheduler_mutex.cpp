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

OS_Mutex_Interface* Scheduler_Mutex_Actions::get_mutex(Squeak_Interpreter* interp) {
  return interp->get_scheduler_mutex();
}

bool Scheduler_Mutex_Actions::is_held(Squeak_Interpreter* interp) {
  return get_mutex(interp)->is_held();
}

void Scheduler_Mutex_Actions::acquire_action(Squeak_Interpreter* interp,
                                             const char* /* why */) {
  OS_Mutex_Interface* mutex = get_mutex(interp);

  // spin and receive to avoid deadlock; other core may be trying to send US something
  Timeout_Timer tt("scheduler mutex", 60, mutex->get_holder()); tt.start();
  while (!mutex->try_lock()) {
    Timeout_Timer::check_all();
    mutex->dec_and_check_recursion_depth();
    Message_Statics::process_any_incoming_messages(false); // could check to ensure no_message
    mutex->check_and_inc_recursion_depth();
  }
  if (tracking) mutex->set_holder(Logical_Core::my_rank());
  
  interp->perf_counter.count_acquire_scheduler_mutex();
}

void Scheduler_Mutex_Actions::release_action(Squeak_Interpreter* interp,
                                             const char*) {
  OS_Mutex_Interface* mutex = get_mutex(interp);
  if (tracking)  mutex->set_holder(-1);
  bool unlocked = mutex->unlock();
  assert_always_msg(unlocked, "Scheduler_Mutex");
}


bool Scheduler_Mutex_Actions::is_initialized(Squeak_Interpreter* interp) {
  return get_mutex(interp)->is_initialized();
}

