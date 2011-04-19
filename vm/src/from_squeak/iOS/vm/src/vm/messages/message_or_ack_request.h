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


struct Message_Or_Ack_Request {
  static inline Message_Or_Ack_Request** first_TL() { return &first_msg_or_req[rank_on_threads_or_zero_on_processes()]; }
  static Message_Or_Ack_Request* first_msg_or_req[Memory_Semantics::max_num_threads_on_threads_or_1_on_processes];
  
  Message_Or_Ack_Request* next;
  Message_Statics::messages msg_type;
  bool is_fulfilled;
  const abstractMessage_class* buf_or_nil_for_ack;
  
  Message_Or_Ack_Request(Message_Statics::messages mt, const abstractMessage_class* b)
  : msg_type(mt), is_fulfilled(false), buf_or_nil_for_ack(b) {
    
    next = *first_TL();
    *first_TL() = this;
    
    assert_eq(Message_Statics::is_encoded_for_ack(mt), buf_or_nil_for_ack == NULL, "buf should be nil iff ack");
  }
  
  ~Message_Or_Ack_Request() { *first_TL() = next; }
  
  static Message_Or_Ack_Request* find(Message_Statics::messages t) {
    for (Message_Or_Ack_Request* mr = *first_TL();  mr != NULL;  mr = mr->next)
      if (!mr->is_fulfilled  &&  t == mr->msg_type)
        return mr;
    
    return NULL;
  }
};
