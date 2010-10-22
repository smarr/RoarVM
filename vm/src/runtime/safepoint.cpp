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


OS_Mutex_Interface* Safepoint_Actions::get_mutex() {
  return The_Squeak_Interpreter()->get_safepoint_mutex();
}

bool Safepoint_Actions::is_held() {
  return get_mutex()->is_held();
}


void Safepoint_Actions::acquire_action(const char* why) {
  OS_Mutex_Interface* mutex = get_mutex();
  mutex->dec_and_check_recursion_depth();
  The_Squeak_Interpreter()->safepoint_tracker->request_other_cores_to_safepoint(why);
  mutex->check_and_inc_recursion_depth();
}

void Safepoint_Actions::release_action(const char* why) {
  The_Squeak_Interpreter()->safepoint_tracker->release_other_cores_from_safepoint(why);
}

bool Safepoint_Actions::is_initialized() {
  return The_Squeak_Interpreter()->is_initialized();
}


void Safepoint_Tracker::print_msg_for_request_safepoint(const char* msg, const char* why) {
  if (Safepoint_Tracker::verbose)
    lprintf("%s %s, depth %d\n",
            msg, why, The_Squeak_Interpreter()->safepoint_tracker->spin_depth());
}


void Safepoint_Tracker::request_other_cores_to_safepoint(const char* why) {
  if (verbose) lprintf("request_other_cores_to_safepoint()\n");
  if (!Safepoint_Ability::is_interpreter_able())
      fatal("deadlock possibility: asking for a safepoint when I am not ready to");
  
  print_msg_for_request_safepoint("requesting safepoint", why);
  assert_always(_am_requesting_other_cores_to_safepoint >= 0);
  ++_am_requesting_other_cores_to_safepoint;
  acquisition_timer.start();
  
  requestSafepointOnOtherCoresMessage_class(why).handle_here_or_send_to(Logical_Core::main_rank);
  
  do {
    // xxxxxx Would things work and be more efficient with true instead false below? -- dmu 4/09
    Message_Statics::process_any_incoming_messages(false);
  } while (!is_every_other_core_safe());
  
  acquisition_timer.stop();
  --_am_requesting_other_cores_to_safepoint;
  assert_always(_am_requesting_other_cores_to_safepoint >= 0);
  print_msg_for_request_safepoint("got safepoint", why);
}


void Safepoint_Tracker::release_other_cores_from_safepoint(const char* why) {
  every_other_core_no_longer_safe();
  releaseOtherCoresFromSafepointMessage_class().handle_here_or_send_to(Logical_Core::main_rank);
  if (verbose)
    lprintf("relinquishing safepoint %s, depth %d\n",
            why, spin_depth());
  if (verbose) lprintf("release_other_cores_from_safepoint() released\n");
  Message_Statics::process_delayed_requests();
}


void Safepoint_Tracker::spin_if_safepoint_requested_with_arguments(const char* fn, const char* file, int line) {
  if (!Safepoint_Ability::is_interpreter_able())
    return;

  // need !is_spinning because otherwise will spin waiting for response message to I am spinning message
  if (does_another_core_need_me_to_spin()  &&  !am_spinning()) {
    if (verbose) lprintf( "spin_if_safepoint_requested about to spin, spin_depth %d\n", spin_depth());
    spin_in_safepoint(fn, file, line);
    if (verbose) lprintf( "spin_if_safepoint_requested done spinning, spin_depth %d\n", spin_depth());
  }
}

void Safepoint_Tracker::another_core_needs_me_to_spin(int for_whom, int seq_no, const char* why) {
  if (verbose) lprintf("another_core_needs_me_to_spin( %d, %d, <%s> )\n", for_whom, seq_no, why);

  if (am_spinning())
    assert_always(_seq_no_of_another_needs_me_to_spin < seq_no);

  _does_another_core_need_me_to_spin = true;
  _which_other_core_needs_me_to_spin = for_whom;
  _seq_no_of_another_needs_me_to_spin = seq_no;
  _why_another_core_needs_me_to_spin = why;
  
  if (am_spinning())
    tell_core_I_am_spinning(seq_no, true);
}

void Safepoint_Tracker::another_core_no_longer_needs_me_to_spin(int seq_no) {
  if (verbose) lprintf("another_core_no_longer_needs_me_to_spin() %d\n", seq_no);
  _does_another_core_need_me_to_spin = false;
  _which_other_core_needs_me_to_spin = -1;
  // maybe it's OK: assert_always_eq(_seq_no_of_another_needs_me_to_spin, seq_no); // I was spinning for a different request!
  _why_another_core_needs_me_to_spin = "";
}


void Safepoint_Tracker::spin_in_safepoint(const char* fn, const char* file,  int line) {
  assert(_spin_depth == 0);
  ++_spin_depth;

  Timeout_Timer tt("spinning in safepoint", 60, Logical_Core::main_rank);
  tt.start();

  tell_core_I_am_spinning(_seq_no_of_another_needs_me_to_spin, false);
  while (does_another_core_need_me_to_spin())
    Message_Statics::process_any_incoming_messages(false);

  --_spin_depth;
}

void Safepoint_Tracker::tell_core_I_am_spinning(int seq_no_of_request, bool was_spinning) {
  tellCoreIAmSpinningMessage_class(seq_no_of_request, was_spinning).handle_here_or_send_to(Logical_Core::main_rank);
}


void Safepoint_Tracker::self_destruct_all() {
  FOR_ALL_OTHER_RANKS(r)  {
    lprintf( "sending destruct to  %d\n", r);
    selfDestructMessage_class("waiting too long for safepoint").send_to(r);
  }
  *(int*)0 = 17;
}





int Safepoint_Master_Control::request_depth = 0;

void Safepoint_Master_Control::request_other_cores_to_safepoint(int requester, const char* why) {
  Top_Level tl;

  ++global_safepoint_request_sequence_number;
  if (verbose)  smc_printf("request_other_cores_to_safepoint(%d, %s, %d)", requester, why,
                           global_safepoint_request_sequence_number);
  cores_asking_for_a_global_safepoint.add(requester, why, global_safepoint_request_sequence_number);

  run();
}


void Safepoint_Master_Control::release_other_cores_from_safepoint(int requester) {
  Top_Level tl;
  if (verbose)  smc_printf("release_other_cores_from_safepoint(%d)", requester);
  assert_always_eq(core_holding_global_safepoint, requester);
  core_holding_global_safepoint = none;

  // since recursion is blocked, must release myself up here so I can run more at top level
  if (spinners.includes(Logical_Core::my_rank()))
    tell_core_to_stop_spinning(Logical_Core::my_rank(), current_safepoint_sequence_number);

  run();
}


void Safepoint_Master_Control::a_core_is_now_safe(int r, int seq_no, bool was_spinning) {
  Top_Level tl;
  if (verbose)  smc_printf("a_core_is_now_safe(%d)", r);
  assert_always_eq(core_holding_global_safepoint, none);
  assert(!cores_asking_for_a_global_safepoint.is_empty());
  
  assert_always_eq(prior_outstanding_spin_requests_sequence_numbers[r], seq_no);
  assert_always_eq(      outstanding_spin_requests_sequence_numbers[r], seq_no);
  
  assert_always(was_spinning || !spinners.includes(r));

  outstanding_spin_requests.remove(r);
  spin_request_timers[r]->stop();
  
  spinners.add(r);
  
  spinners_sequence_numbers[r] = prior_outstanding_spin_requests_sequence_numbers[r]; outstanding_spin_requests_sequence_numbers[r] = -1; 
  
  run();
}



void Safepoint_Master_Control::run() {
  // Cannot just ask all needed spinners to spin because things change while waiting for the answer
  // Instead loop till outstanding requests matches what's needed
  while (step());
}


bool Safepoint_Master_Control::step() {
  // Cannot just ask all needed spinners to spin because things change while waiting for the answer
  // Do one step, return true if did something
  if (verbose)  smc_printf("step");
  if (step_recurse_level > 0  &&  !spinners.includes(Logical_Core::my_rank()) /* might never run if I am spinning*/) {
    return false;
  }

  ++step_recurse_level;

  bool r =
    core_holding_global_safepoint != none           ?  false :
   !cores_asking_for_a_global_safepoint.is_empty()  ?  step_towards_granting()  :
                                                       maybe_release_a_spinner();
  --step_recurse_level;
  return r;
}


bool Safepoint_Master_Control::step_towards_granting() {

  Rank_Set spinners_needed_for_next_grantee = all_cores - next_grantee();

  if (verbose)  smc_printf("step_towards_granting: spinners_needed_for_next_grantee 0x%llx", spinners_needed_for_next_grantee.contents());

  // must ask myself to spin last, otherwise, recursion block will prevent me from finishing safepoint

  return maybe_stop_a_spinner()
  ||     maybe_cancel_a_spin_request()
  ||     maybe_make_a_spin_request_to_another_core(spinners_needed_for_next_grantee)
  ||     maybe_ask_myself_to_spin()
  ||     maybe_grant_safepoint_to_next_requester(  spinners_needed_for_next_grantee);
}

bool Safepoint_Master_Control::maybe_stop_a_spinner() {
  if (verbose)  smc_printf("maybe_stop_a_spinner");
  if ( !spinners.includes(next_grantee()))
    return false;
  tell_core_to_stop_spinning( next_grantee(), next_sequence_number());
  return true;
}

bool Safepoint_Master_Control::maybe_cancel_a_spin_request() {
  if (verbose)  smc_printf("maybe_cancel_a_spin_request");
  if ( !outstanding_spin_requests.includes(next_grantee()))
    return false;
  tell_core_to_stop_spinning(next_grantee(), next_sequence_number());
  return true;
}

bool Safepoint_Master_Control::maybe_make_a_spin_request_to_another_core(Rank_Set spinners_needed_for_next_grantee) {
  if (verbose)  smc_printf("maybe_make_a_spin_request_to_another_core");
  int  first_spin_request_needed = (spinners_needed_for_next_grantee - (spinners + outstanding_spin_requests) - Logical_Core::my_rank()).first_or_none();
  if ( first_spin_request_needed == Rank_Set::none )
    return false;

  request_core_to_spin(first_spin_request_needed, next_grantee(), next_sequence_number(), next_grantee_why());
  return true;
}

bool Safepoint_Master_Control::maybe_ask_myself_to_spin() {
  if (verbose)  smc_printf("maybe_ask_myself_to_spin");
  if ((spinners + outstanding_spin_requests).includes(Logical_Core::my_rank())  ||  next_grantee() == Logical_Core::my_rank())
    return false;

  request_core_to_spin(Logical_Core::my_rank(), next_grantee(), next_sequence_number(), next_grantee_why());
  return true;
}

bool Safepoint_Master_Control::maybe_grant_safepoint_to_next_requester(Rank_Set spinners_needed_for_next_grantee) {
  if (verbose)  smc_printf("maybe_grant_safepoint_to_next_requester");
  if (spinners_needed_for_next_grantee != spinners)
    return false;

  grant_safepoint_to_next_asker();
  return true;
}

bool Safepoint_Master_Control::maybe_release_a_spinner() {
  if (verbose)  smc_printf("maybe_release_a_spinners()");
  int r = spinners.first_or_none();
  if (r == Rank_Set::none) return false;
  tell_core_to_stop_spinning(r, current_safepoint_sequence_number);
  return true;
}





void Safepoint_Master_Control::request_core_to_spin(int r, int for_whom, int seq_no, const char* why) {
  if (verbose)  smc_printf("request_core_to_spin(%d, %d, %d, %s)", r, for_whom, seq_no, why);
  if  (cores_asking_for_a_global_safepoint.is_empty())
    fatal("should not be asking for a spin");
  outstanding_spin_requests.add(r);
  prior_outstanding_spin_requests_sequence_numbers[r] = outstanding_spin_requests_sequence_numbers[r] = seq_no;
  spin_request_timers[r]->start();
  requestCoreToSpinMessage_class(for_whom, seq_no, why).handle_here_or_send_to(r);
}

void Safepoint_Master_Control::tell_core_to_stop_spinning(int r, int seq_no) {
  if (verbose)  smc_printf("tell_core_to_stop_spinning(%d)", r);
  assert(r < Logical_Core::group_size);
  outstanding_spin_requests.remove(r); // may be redundant
  spinners.remove(r);
  tellCoreToStopSpinningMessage_class(seq_no).handle_here_or_send_to(r);
}

void Safepoint_Master_Control::grant_safepoint_to_next_asker() {
  // lprintf("grant_safepoint_to_next_asker(%d %d(%s))\n", next_grantee(), next_sequence_number(), next_grantee_why());
  // if (verbose)  smc_printf("grant_safepoint_to_next_asker(%d %d(%s))", next_grantee(), next_sequence_number(), next_grantee_why());
  assert_always_eq(core_holding_global_safepoint, none);
  core_holding_global_safepoint = next_grantee();
  current_safepoint_sequence_number = next_sequence_number();
  cores_asking_for_a_global_safepoint.remove();
  
  if (verbose)
    lprintf("Interactions::grant_safepoint_to(%d), seq_no(%d)\n",
            core_holding_global_safepoint, current_safepoint_sequence_number);
  
  grantSafepointMessage_class(current_safepoint_sequence_number).handle_here_or_send_to(core_holding_global_safepoint);
}



void Safepoint_Master_Control::smc_printf(const char* msg, ...) {
  va_list ap;
  va_start(ap, msg);
  smc_vprintf(msg, ap);
  va_end(ap);
}

void Safepoint_Master_Control::smc_vprintf(const char* msg, va_list args) {
  char buf[BUFSIZ];
  char ps_buf[BUFSIZ];
  print_string(ps_buf, sizeof(ps_buf));
  snprintf(buf, sizeof(buf),
           "%*s%s: %s\n", request_depth * 2, "", msg, ps_buf);
  ::vlprintf(buf, args);
}


void Safepoint_Master_Control::print_string(char* buf, int buf_size) {
  char cafgs_buf[BUFSIZ];
  cores_asking_for_a_global_safepoint.print_string(cafgs_buf, sizeof(cafgs_buf));
  snprintf(buf, buf_size,
           "Safepoint_Master_Control: askers: %s, open spin reqs 0x%llx, "
           "spinners 0x%llx, winner %d, seq_no %d:  ",
           cafgs_buf, outstanding_spin_requests.contents(),
           spinners.contents(), core_holding_global_safepoint, current_safepoint_sequence_number);
}

void Safepoint_Master_Control::smc_white_space() { if (verbose) lprintf("\n\n"); }

