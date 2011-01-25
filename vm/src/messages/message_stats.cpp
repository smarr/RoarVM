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


cacheline_aligned<int>     Message_Stats::send_tallies[Memory_Semantics::max_num_threads_on_threads_or_1_on_processes][Message_Statics::end_of_messages];
cacheline_aligned<int>     Message_Stats::receive_tallies[Memory_Semantics::max_num_threads_on_threads_or_1_on_processes][Message_Statics::end_of_messages];
cacheline_aligned<u_int64> Message_Stats::receive_cycles[Memory_Semantics::max_num_threads_on_threads_or_1_on_processes][Message_Statics::end_of_messages];
cacheline_aligned<u_int64> Message_Stats::buf_msg_check_cyc[Memory_Semantics::max_num_threads_on_threads_or_1_on_processes] = { 0LL };
cacheline_aligned<int>     Message_Stats::buf_msg_check_count[Memory_Semantics::max_num_threads_on_threads_or_1_on_processes] = { 0 };


Oop Message_Stats::get_stats(int what_to_sample) {
  int rank_on_threads_or_zero_on_processes = Memory_Semantics::rank_on_threads_or_zero_on_processes();

  int s = The_Squeak_Interpreter()->makeArrayStart();

  if (what_to_sample & (1 << SampleValues::coreCoords)) {
# if On_Tilera
    int x    = CPU_Coordinate::my_x(),  y = CPU_Coordinate::my_y();
# else
    int x    = Logical_Core::my_rank(), y = 0;
# endif
    int rank = Logical_Core::my_rank();
    PUSH_POSITIVE_32_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(x);
    PUSH_POSITIVE_32_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(y);
    PUSH_POSITIVE_32_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(rank);
  }

  if (what_to_sample & (1 << SampleValues::sendTallies)) {
    int r = The_Squeak_Interpreter()->makeArrayStart();
    for (int j = 0;  j < Message_Statics::end_of_messages;  ++j)
      PUSH_POSITIVE_32_BIT_INT_FOR_MAKE_ARRAY(send_tallies[rank_on_threads_or_zero_on_processes][j].value);
    bzero(send_tallies[rank_on_threads_or_zero_on_processes], sizeof(send_tallies[rank_on_threads_or_zero_on_processes]));
    Oop sendTallies = The_Squeak_Interpreter()->makeArray(r);
    PUSH_WITH_STRING_FOR_MAKE_ARRAY(sendTallies);
  }
  if (what_to_sample & (1 << SampleValues::receiveTallies)) {
    int r = The_Squeak_Interpreter()->makeArrayStart();
    for (int j = 0;  j < Message_Statics::end_of_messages;  ++j)
      PUSH_POSITIVE_32_BIT_INT_FOR_MAKE_ARRAY(receive_tallies[rank_on_threads_or_zero_on_processes][j].value);
    bzero(receive_tallies[rank_on_threads_or_zero_on_processes], sizeof(receive_tallies[rank_on_threads_or_zero_on_processes]));
    Oop receiveTallies = The_Squeak_Interpreter()->makeArray(r);
    PUSH_WITH_STRING_FOR_MAKE_ARRAY(receiveTallies);
  }
  if (what_to_sample & (1 << SampleValues::bufferedMessageStats)) {
    PUSH_POSITIVE_64_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(buf_msg_check_cyc[rank_on_threads_or_zero_on_processes].value); buf_msg_check_cyc[rank_on_threads_or_zero_on_processes].value = 0LL;
    PUSH_POSITIVE_32_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(buf_msg_check_count[rank_on_threads_or_zero_on_processes].value); buf_msg_check_count[rank_on_threads_or_zero_on_processes].value = 0;
  }
  if (what_to_sample & (1 << SampleValues::receiveCycles)) {
    int r = The_Squeak_Interpreter()->makeArrayStart();
    for (int j = 0;  j < Message_Statics::end_of_messages;  ++j)
      PUSH_POSITIVE_64_BIT_INT_FOR_MAKE_ARRAY(receive_cycles[rank_on_threads_or_zero_on_processes][j].value);
    bzero(receive_cycles[rank_on_threads_or_zero_on_processes], sizeof(receive_cycles[rank_on_threads_or_zero_on_processes]));
    Oop receiveCycles = The_Squeak_Interpreter()->makeArray(r);
    PUSH_WITH_STRING_FOR_MAKE_ARRAY(receiveCycles);
  }
  return The_Squeak_Interpreter()->makeArray(s);
}


Oop Message_Stats::get_message_names() {
  int r = The_Squeak_Interpreter()->makeArrayStart();
  for (int i = 0;  i < Message_Statics::end_of_messages; ++i)
    PUSH_STRING_FOR_MAKE_ARRAY(Message_Statics::message_names[i]);
  return The_Squeak_Interpreter()->makeArray(r);
}


# if Check_Reliable_At_Most_Once_Message_Delivery

cacheline_aligned<int> Message_Stats::next_transmission_serial_number[Message_Statics::end_of_messages][Max_Number_Of_Cores][Memory_Semantics::max_num_threads_on_threads_or_1_on_processes];
cacheline_aligned<int> Message_Stats::next_receive_serial_number[Message_Statics::end_of_messages][Max_Number_Of_Cores][Memory_Semantics::max_num_threads_on_threads_or_1_on_processes];


void Message_Stats::check_received_transmission_sequence_number(Message_Statics::messages msg_type, int tsn, int sender) {
  int should_be = next_receive_serial_number[msg_type][sender][rank_on_threads_or_zero_on_processes()].value++;
  if (tsn != should_be) {
    lprintf("check_received_transmission_sequence_number: message %s from %d to %d is %d should_be %d\n",
            Message_Statics::message_names[msg_type], sender, Logical_Core::my_rank(), tsn, should_be);
    fatal("message delivery error");
  }
}
# endif


