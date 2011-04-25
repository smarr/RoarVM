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
<<<<<<< HEAD
=======

>>>>>>> f4a658a7aeee0ea2d5d8612d75dfb6f910865e62
public:

  typedef struct statistics {
    int     send_tallies   [Message_Statics::end_of_messages];
    int     receive_tallies[Message_Statics::end_of_messages];
    u_int64 receive_cycles [Message_Statics::end_of_messages];
    u_int64 buf_msg_check_cyc;
    int     buf_msg_check_count;
  } statistics;
  
  
  static statistics stats[Memory_Semantics::max_num_threads_on_threads_or_1_on_processes];    // threadsafe
<<<<<<< HEAD
  
=======

>>>>>>> f4a658a7aeee0ea2d5d8612d75dfb6f910865e62
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
    ++Message_Stats::stats[rank_on_threads_or_zero_on_processes()].send_tallies[m];
  };
  
  static void collect_receive_msg_stats(int m) {
    ++Message_Stats::stats[rank_on_threads_or_zero_on_processes()].receive_tallies[m];
<<<<<<< HEAD
  };  
=======
  };
>>>>>>> f4a658a7aeee0ea2d5d8612d75dfb6f910865e62
  
# if  Check_Reliable_At_Most_Once_Message_Delivery
  static int next_transmission_serial_number[Message_Statics::end_of_messages][Max_Number_Of_Cores][Memory_Semantics::max_num_threads_on_threads_or_1_on_processes];
  static int next_receive_serial_number[Message_Statics::end_of_messages][Max_Number_Of_Cores][Memory_Semantics::max_num_threads_on_threads_or_1_on_processes];
  static void check_received_transmission_sequence_number(Message_Statics::messages, int, int);
# endif
  
  
};

