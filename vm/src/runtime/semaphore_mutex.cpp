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

// Templates don't work with Tilera -- dmu 4/09


OS_Mutex_Interface* Semaphore_Mutex_Actions::get_mutex() {
  return The_Squeak_Interpreter()->get_semaphore_mutex();
}

bool Semaphore_Mutex_Actions::is_held() {
  return get_mutex()->is_held();
}

void Semaphore_Mutex_Actions::acquire_action(const char*) {
  // xxxxxx This code could/should be better factored across the various mutexes. -- dmu 4/09

  // spin and receive to avoid deadlock; other core may be trying to send US something

  OS_Mutex_Interface* mutex = get_mutex();

  assert_always(!mutex->is_held()); // deadlock check
  Timeout_Timer tt("semaphore mutex", check_assertions ? 300 : 60, mutex->get_holder()); tt.start();
  while (0 != mutex->try_lock()) {
    Timeout_Timer::check_all();
    mutex->dec_and_check_recursion_depth();
    Message_Statics::process_any_incoming_messages(false); // could check to ensure no_message
    mutex->check_and_inc_recursion_depth();
  }
  if (tracking) mutex->set_holder(Logical_Core::my_rank());

}

void Semaphore_Mutex_Actions::release_action(const char*) {
  OS_Mutex_Interface* mutex = get_mutex();
  if (tracking)  mutex->set_holder(-1);
  OS_Interface::abort_if_error("Semaphore_Mutex", mutex->unlock());
}


bool Semaphore_Mutex_Actions::is_initialized() {
  return The_Squeak_Interpreter()->is_initialized();
}

