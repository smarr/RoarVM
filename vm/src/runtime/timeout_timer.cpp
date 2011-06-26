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


void Timeout_Timer::check() {
  //static const char* check_for = NULL; // "addedScheduledProcessResponse";
  if (!has_timed_out()  ||  Timeout_Deferral::are_timeouts_deferred())
    return;
  complain();
  stop();
  // if (strcmp("spinning in safepoint", why) == 0)
  //   for (;;);
  act();
}


void Timeout_Timer::restart() {
  if (!is_running())  return;
  start();
}


void Timeout_Timer::complain() {
  if (strcmp(why, Message_Statics::message_names[Message_Statics::runPrimitiveResponse]) == 0 )
    lprintf("timed out waiting for %s from %d after %d secs, remote_prim_fn: 0x%x\n",
            why, who_I_am_waiting_for, elapsed_seconds(), Message_Statics::remote_prim_fn);

  else
    lprintf("timed out waiting for %s from %d after %d secs\n",
            why, who_I_am_waiting_for, elapsed_seconds());
}

void Timeout_Timer::act() {
  if (who_I_am_waiting_for != any) {
    lprintf( "sending destruct\n");
    selfDestructMessage_class("cause of my timoeout").send_to(who_I_am_waiting_for);
    kill(who_I_am_waiting_for - Logical_Core::my_rank() + getpid(), SIGBUS);
  }
  for (;;);
  fatal("timeout");
}

void Timeout_Timer::check_all() {
  FOR_ALL_TIMEOUT_TIMERS(p) p->check();
}

void Timeout_Timer::restart_all() {
  FOR_ALL_TIMEOUT_TIMERS(p) p->restart();
}

void Safepoint_Acquisition_Timer::complain() {
  lprintf( "too long to get safepoint: %ld s\n", elapsed_seconds());
}

void Safepoint_Acquisition_Timer::act() {
  lprintf( "sending destruct to the holdouts\n");
  The_Squeak_Interpreter()->safepoint_tracker->self_destruct_all();
  for(;;);
}

