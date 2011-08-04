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


class abstractMessage_class {
protected:
  enum acking { 
    no_ack,
    post_ack_for_correctness, 
  };
  bool should_ack(bool, int);
  
  enum safepoint_delay_options { delay_when_have_acquired_safepoint, dont_delay_when_have_acquired_safepoint} ;
  virtual acking get_ack_setting() const = 0;
  virtual safepoint_delay_options get_safepoint_delay_setting() const = 0;
  
  void defer_till_done_with_safepoint();
  
public:
  abstractMessage_class(                ) { sender = cpu_core_my_rank(); }
  abstractMessage_class( Receive_Marker*) { }
  virtual void send_to(int);
  virtual void send_to_GC();
  virtual Message_Statics::messages get_message_type() const = 0;
  void send_to_other_cores();
  void send_to_all_cores();
  void handle_here_or_send_to(int r);
  virtual int size_for_transmission_and_copying() const { return sizeof(*this); }
  
  void send_then_receive_and_handle_messages_returning_a_match(int, const abstractMessage_class&);
  void receive_and_handle_messages_returning_a_match(int) const;
  
  Message_Statics::messages header;
  int sender;
  
  
# if Checksum_Messages
  int checksum;
  int compute_checksum();
# endif
  
# if  Check_Reliable_At_Most_Once_Message_Delivery
  int transmission_serial_number;
  static int* next_transmission_serial_number_by_class;
# endif
  
  void ack_if_appropriate(bool);
  virtual void do_all_roots(Oop_Closure*) {}
    
  virtual void handle_me() = 0;
  virtual void handle_me_and_ack();
  virtual void handle_me_or_maybe_delay();
  
  virtual void print();
  
};
