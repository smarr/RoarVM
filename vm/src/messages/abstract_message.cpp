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

# if Checksum_Messages

int abstractMessage_class::compute_checksum() {
  int r = checksum; // this way checksum gets xored with itself, so ignored
  int n = size_for_transmission_and_copying();
  for (int* p = (int*)this;  (char*)p - (char*)this  <  n;  r ^= *p++);
  return r;
}
# endif



bool abstractMessage_class::should_ack(bool has_been_handled, int receiver_rank) {
  switch (get_ack_setting()) {
    default:                          return fatal("???");
    case no_ack:                      return false;
    case post_ack_for_correctness:    return sender != receiver_rank  &&  has_been_handled;
  }
}


void abstractMessage_class::send_then_receive_and_handle_messages_returning_a_match(int rank, const abstractMessage_class& msg_to_rcv) {
  if  ( get_safepoint_delay_setting() == delay_when_have_acquired_safepoint
       &&    !Safepoint_Ability::is_interpreter_able() )  {
    lprintf("About to wait for a response(%s) to a message(%s) that won't be processed if %d acquires a safepoint, "
            "but this core is unable to safepoint, so deadlock will result.\n",
            Message_Statics::message_names[msg_to_rcv.get_message_type()], Message_Statics::message_names[get_message_type()], rank);
    fatal("Deadlock possibility");
  }
  send_to(rank);
  msg_to_rcv.receive_and_handle_messages_returning_a_match(rank);
}

void abstractMessage_class::receive_and_handle_messages_returning_a_match(int from_rank) const {
  Message_Statics::receive_and_handle_messages_returning_a_match(get_message_type(), this, from_rank);
}


void abstractMessage_class::send_to(int r) {
  assert_always(r != Logical_Core::my_rank()); // should not get here
  if (get_safepoint_delay_setting() == delay_when_have_acquired_safepoint  &&  !Safepoint_Ability::is_interpreter_able()) 
    fatal("Deadlock possible: I may wait for this message to be handled, but if the other core is trying to safepoint, I won't allow it to");
  
# if Checksum_Messages
  checksum = compute_checksum();
# endif
# if  Check_Reliable_At_Most_Once_Message_Delivery
  // must use get_message_type() cause ackMessage encodes orig type in header -- dmu 5/10
  transmission_serial_number = Message_Stats::next_transmission_serial_number[get_message_type()][r][rank_on_threads_or_zero_on_processes()]++;
# endif
  
  assert(r < Max_Number_Of_Cores);
  logical_cores[r].message_queue.send_message(this);
  if (should_ack( false, r)
      ||  should_ack(  true, r))
    Message_Statics::wait_for_ack(header, r);
}

void abstractMessage_class::handle_here_or_send_to(int r) {
  if (r == Logical_Core::my_rank()) handle_me();
  else send_to(r);
}


void abstractMessage_class::send_to_other_cores() {
  FOR_ALL_OTHER_RANKS(i)
    send_to(i);
}

void abstractMessage_class::send_to_all_cores() {
  handle_me();
  send_to_other_cores();
}


void abstractMessage_class::ack_if_appropriate(bool has_been_handled) {
  
  if (should_ack( has_been_handled, Logical_Core::my_rank()))
    ackMessage_class(get_message_type()).send_to(sender); 
}


void abstractMessage_class::defer_till_done_with_safepoint() {
  if (The_Squeak_Interpreter()->safepoint_tracker->am_acquiring_safepoint_while_spinning_for(sender)) {
    lprintf("Deadlock possible: received a message %s from core %d which must be delayed while I am acquiring a safepoint, "
            "but it is the very same core that has safepointed me because %s\n",
            Message_Statics::message_names[get_message_type()], sender, The_Squeak_Interpreter()->safepoint_tracker->why_other_core_needs_me_to_spin());
    fatal("DEADLOCK");
  }
  Deferred_Request::add(this);
}


void abstractMessage_class::handle_me_and_ack() { 
  ack_if_appropriate(false); 
  handle_me_or_maybe_delay(); 
  ack_if_appropriate(true); 
} 

void abstractMessage_class::handle_me_or_maybe_delay() { 
  if (get_safepoint_delay_setting() == delay_when_have_acquired_safepoint 
      &&  The_Squeak_Interpreter()->safepoint_tracker->have_acquired_safepoint()) 
    defer_till_done_with_safepoint(); 
  else  { 
    u_int64 start = OS_Interface::get_cycle_count(); 
    handle_me(); 
    u_int64 end = OS_Interface::get_cycle_count();
    Message_Stats::record_receive_cycles(get_message_type(), end - start);
  } 
}

void abstractMessage_class::print() { 
  lprintf(" msg: %s\n", Message_Statics::message_names[get_message_type()]);
}
