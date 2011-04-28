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


class abstractMessage_class;

class Message_Statics {
public:
  
# define MAKE_ENUM(name,superclass,carg,scarg,conbody,body, ack, safepoint_delay_setting) name,
  
  enum messages {
    FOR_ALL_MESSAGES_DO(MAKE_ENUM)
    end_of_messages
  };
# undef MAKE_ENUM
  
  static messages encode_msg_type_for_ack(messages t) { return messages(t + end_of_messages); }
  static messages decode_msg_type_for_ack(messages t) { return messages(t - end_of_messages); }
  static bool     is_encoded_for_ack(messages t)      { return t >= end_of_messages; }
  
  static const char* message_names[];
  
  static size_t max_message_size();
  static void print_size();
  
  static void process_any_incoming_messages(bool);
  static void receive_and_handle_messages_returning_a_match(messages msg, const abstractMessage_class*, int);
  static bool receive_and_handle_one_message(bool wait_for_msg);
  static void wait_for_ack(messages, int);
  static void process_delayed_requests();
  
public:
  
  static const bool verbose = false;
  static       bool run_timer;
  
  static fn_t remote_prim_fn; // for debugging
  
};
