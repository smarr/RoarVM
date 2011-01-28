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


class Message_Stats {
  Message_Stats() { fatal("Message_Stats is not meant to be instanciated."); }

public:

  static cacheline_aligned<int>     send_tallies[Memory_Semantics::max_num_threads_on_threads_or_1_on_processes][Message_Statics::end_of_messages];    // threadsafe
  static cacheline_aligned<int>     receive_tallies[Memory_Semantics::max_num_threads_on_threads_or_1_on_processes][Message_Statics::end_of_messages]; // threadsafe
  static cacheline_aligned<u_int64> receive_cycles[Memory_Semantics::max_num_threads_on_threads_or_1_on_processes][Message_Statics::end_of_messages];  // threadsafe
  static cacheline_aligned<u_int64> buf_msg_check_cyc[Memory_Semantics::max_num_threads_on_threads_or_1_on_processes];                          // threadsafe
  static cacheline_aligned<int>     buf_msg_check_count[Memory_Semantics::max_num_threads_on_threads_or_1_on_processes];                        // threadsafe

  static Oop get_stats(int);
  static Oop get_message_names();

  static void collect_send_msg_stats(int m) {
    const bool verbose = false;
    if (verbose)
      switch(m) {
          // case Message_Statics::requestSafepointMessage:
          // case Message_Statics::noMessage:
        case Message_Statics::addObjectFromSnapshotMessage:
        case Message_Statics::addObjectFromSnapshotResponse:
        case Message_Statics::broadcastInterpreterDatumMessage:
          break;
          
        default: lprintf( "->%d sending %d %s\n", cpu_core_my_rank(), m, Message_Statics::message_names[m]); break;
      }
    ++Message_Stats::send_tallies[rank_on_threads_or_zero_on_processes()][m].value;
  };
  
  static void collect_receive_msg_stats(int m) {
    ++Message_Stats::receive_tallies[rank_on_threads_or_zero_on_processes()][m].value;
  };
  
# if  Check_Reliable_At_Most_Once_Message_Delivery
  static cacheline_aligned<int> next_transmission_serial_number[Message_Statics::end_of_messages][Max_Number_Of_Cores][Memory_Semantics::max_num_threads_on_threads_or_1_on_processes];
  static cacheline_aligned<int> next_receive_serial_number[Message_Statics::end_of_messages][Max_Number_Of_Cores][Memory_Semantics::max_num_threads_on_threads_or_1_on_processes];
  static void check_received_transmission_sequence_number(Message_Statics::messages, int, int);
# endif
  
  
};

