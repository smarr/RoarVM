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


# include "headers.h"


fn_t Message_Statics::remote_prim_fn = NULL;
bool Message_Statics::run_timer = false;       // config flag

# define MAKE_STRING(name, superclass, formals, args, ctor_body, body, ack, safepoint_delay_setting) #name,
const char* Message_Statics::message_names[] = {
  FOR_ALL_MESSAGES_DO(MAKE_STRING)
  NULL
};
# undef MAKE_STRING

void Message_Statics::process_delayed_requests() {
  Deferred_Request::service_and_free_all();
}


// must do something special so that if I recurse:
/*
 #0  0x0016b918 in receive_and_handle_messages_returning_a_match__8MessagesSGbQ2_8Messages8messagesPv (wait=true, msg=flushByMethodResponse__8Messages,
 msg_obj=0xbe37cf50) at ../src/messages/messages.cpp:97
 #1  0x00227a60 in abstractMessage_class::receive_and_handle_messages_returning_a_match (this=0xbe37cf50, wait=true, msg=flushByMethodResponse__8Messages)
 at /home/ungar/renaissance/rvm/src/messages/messages.h:145
 #2  0x001785a8 in Interactions::flushByMethod_on_all_tiles (this=0x3d2dbc, x={_bits = 1101863232}) at ../src/messages/interactions.cpp:482
 #3  0x000aa638 in Squeak_Interpreter::primitiveFlushCacheByMethod (this=0x3ca930) at ../src/interpreter/interpreter_primitives.cpp:741
 #4  0x000fd418 in primitiveFlushCacheByMethod () at ../src/interpreter/primitive_table.cpp:267
 #5  0x0014e228 in dispatchFunctionPointer__18Squeak_InterpreterGPGve_Pvb (this=0x3ca930, f=0xfd3c0 <primitiveFlushCacheByMethod(void,...)>, on_main=false)
 at ../src/interpreter/squeak_interpreter.cpp:1966
 #6  0x0014e048 in run_primitive_on_main_from_elsewhere__18Squeak_InterpreterGPGve_Pv (this=0x3ca930, f=0xfd3c0 <primitiveFlushCacheByMethod(void,...)>)
 at ../src/interpreter/squeak_interpreter.cpp:1956
 #7  0x00176d60 in runPrimitiveMessage_class::handle_me (this=0xbe38459c) at ../src/messages/messages.cpp:515
 #8  0x0016e750 in receive_and_handle_one_raw_message_returning_a_match__8MessagesSGbQ2_8Messages8messagesPv (wait=64, msg=1104347728, msg_obj=0xbe38459c)
 at ../src/messages/messages.cpp:182
 #9  0x0016bb40 in receive_and_handle_one_message_returning_a_match__8MessagesSGbQ2_8Messages8messagesPv (wait=false, msg=runPrimitiveMessage__8Messages,
 msg_obj=0xbadbad00) at ../src/messages/messages.cpp:124
 #10 0x0016b9a8 in receive_and_handle_messages_returning_a_match__8MessagesSGbQ2_8Messages8messagesPv (wait=true,
 msg=broadcastInterpreterDatumResponse__8Messages, msg_obj=0xbe38e444) at ../src/messages/messages.cpp:102
 #11 0x00227a60 in abstractMessage_class::receive_and_handle_messages_returning_a_match (this=0xbe38e444, wait=true,
 msg=broadcastInterpreterDatumResponse__8Messages) at /home/ungar/renaissance/rvm/src/messages/messages.h:145
 #12 0x0017aa40 in broadcast_interpreter_datum__12InteractionsGiPvUL (this=0x3d2dbc, datum_size=4, datum_addr=0x3ceaf4, datum=1)
 at ../src/messages/interactions.cpp:576
 #13 0x0017a720 in Interactions::broadcast_interpreter_int32 (this=0x3d2dbc, d=0x3ceaf4) at ../src/messages/interactions.cpp:36
 #14 0x00150cc0 in Squeak_Interpreter::signalSemaphoreWithIndex (this=0x3ca930, index=1) at ../src/interpreter/squeak_interpreter.cpp:807
 #15 0x00145240 in signalSemaphoreWithIndex (i=1) at ../src/runtime/squeak_adapters.cpp:367
 #16 0x0011f308 in signalInputEvent () at /home/ungar/renaissance/rvm/src/from_squeak/unix/vm/sqUnixEvent.c:153
 #17 0x0011f4f8 in recordMouseEvent () at /home/ungar/renaissance/rvm/src/from_squeak/unix/vm/sqUnixEvent.c:167
 #18 0x00127de0 in handleEvent (evt=0xbe38e5b8) at ../src/from_squeak/unix/vm-display-X11/sqrUnixX11.c:1297
 #19 0x00128df0 in handleEvents () at ../src/from_squeak/unix/vm-display-X11/sqrUnixX11.c:1483
 #20 0x0012f128 in display_ioProcessEvents () at ../src/from_squeak/unix/vm-display-X11/sqrUnixX11.c:2321
 #21 0x00118928 in ioProcessEvents () at ../src/from_squeak/unix/vm/sqrUnixMain.c:536
 #22 0x0011f978 in display_ioGetNextEvent (evt=0xbe38e674) at /home/ungar/renaissance/rvm/src/from_squeak/unix/vm/sqUnixEvent.c:227
 #23 0x00119788 in ioGetNextEvent (evt=0xbe38e674) at ../src/from_squeak/unix/vm/sqrUnixMain.c:585
 #24 0x000ab700 in Squeak_Interpreter::primitiveGetNextEvent (this=0x3ca930) at ../src/interpreter/interpreter_primitives.cpp:821
 #25 0x000fd918 in primitiveGetNextEvent () at ../src/interpreter/primitive_table.cpp:267
 #26 0x0014e228 in dispatchFunctionPointer__18Squeak_InterpreterGPGve_Pvb (this=0x3ca930, f=0xfd8c0 <primitiveGetNextEvent(void,...)>, on_main=false)
 at ../src/interpreter/squeak_interpreter.cpp:1966
 #27 0x0014e048 in run_primitive_on_main_from_elsewhere__18Squeak_InterpreterGPGve_Pv (this=0x3ca930, f=0xfd8c0 <primitiveGetNextEvent(void,...)>)
 at ../src/interpreter/squeak_interpreter.cpp:1956
 #28 0x00176d60 in runPrimitiveMessage_class::handle_me (this=0xbe38459c) at ../src/messages/messages.cpp:515
 #29 0x0016e750 in receive_and_handle_one_raw_message_returning_a_match__8MessagesSGbQ2_8Messages8messagesPv (wait=64, msg=1104347728, msg_obj=0xbe38459c)
 at ../src/messages/messages.cpp:182
 
 */
// I won't lock up forever.
// But only need to handle one type of message at a time

int incoming_msg_count = 0; // for debugging
void Message_Statics::process_any_incoming_messages(bool wait) {
  // incoming_msg_count just for debugging
  for ( incoming_msg_count = 0;  receive_and_handle_one_message(wait);  wait = false, ++incoming_msg_count) ;
}


// Handle case where while waiting for A, we recursively get a call to wait for B
// Must not release A, but rather return it eventually
void Message_Statics::receive_and_handle_messages_returning_a_match(messages msg, const abstractMessage_class* result_or_nil_for_ack, int from_rank) {
  Message_Or_Ack_Request mr(msg, result_or_nil_for_ack); // must live through this stack frame
  Timeout_Timer tt(message_names[msg], 60, from_rank);
  if  ( Message_Statics::run_timer )
    tt.start();
  do  {
    // Would things work and be more efficient with true instead false below? -- dmu 4/09
    // Nope, initial snapshot never loads for some reason. -- dmu 6/10
    receive_and_handle_one_message(false);
    The_Squeak_Interpreter()->safepoint_tracker->spin_if_safepoint_requested();
    // old deadlock detection code used to go here after an else
  }
  while (!mr.is_fulfilled);
}




size_t Message_Statics::max_message_size() {
  size_t r = 0;
# define GET_MAX_SIZE(name, superclass, constructor_formals, superconstructor_actuals, constructor_body, class_body, ack_setting, safepoint_delay_setting) \
  r = max(r, sizeof(name##_class));
  
  FOR_ALL_MESSAGES_DO(GET_MAX_SIZE)
# undef GET_MAX_SIZE
  
  return r;
}

// Used only for debugging and optimizing message buffer sizes
# define PRINT_SIZE(name, superclass, formals, args, ctor_body, body, ack, safepoint_delay_setting) lprintf("MsgClass %s has size %d\n", #name, sizeof(name##_class)); 
void Message_Statics::print_size() {
  FOR_ALL_MESSAGES_DO(PRINT_SIZE);
}
# undef PRINT_SIZE


// Receive a message, handle it, store it into one of the Message_Request buffers if there is a matching request in the chain.
// If wait is true, don't return until a message has been seen.
// Return true if we got a message.
Message_Statics::messages last_msg_type = Message_Statics::noMessage; // for debugging

bool Message_Statics::receive_and_handle_one_message(bool wait) {
  Squeak_Interpreter* const interp = The_Squeak_Interpreter();
  const int rank_on_threads_or_zero_on_processes = interp->rank_on_threads_or_zero_on_processes();
  
  // Without this check, if the spin request has already been received,
  // any message received below could have roots that would be missed by GC -- dmu 10/1/10
  interp->safepoint_tracker->spin_if_safepoint_requested(); 
  
  abstractMessage_class* buffered_msg;
  Logical_Core* buffer_owner;
  messages msg_type_or_encoded_acking_type;
  
  messages msg_type;
  for (;;) {
    // added this loop and pass in false to receiver prims below in order to do timeout checks
    const bool do_timeout_checks = true; //  tried false but no speedup loading image -- dmu 6/10

    u_int64 start = OS_Interface::get_cycle_count();
    buffered_msg = (abstractMessage_class*)Message_Queue::buffered_receive_from_anywhere(wait && !do_timeout_checks, &buffer_owner, interp->my_core());
    u_int64 end = OS_Interface::get_cycle_count();
    
    Message_Stats::record_buffered_recieve(rank_on_threads_or_zero_on_processes, end - start);
    
    msg_type_or_encoded_acking_type = buffered_msg == NULL  ?  noMessage  :  buffered_msg->header;
    
    msg_type = is_encoded_for_ack(msg_type_or_encoded_acking_type) ? ackMessage : msg_type_or_encoded_acking_type;
    
# if  Check_Reliable_At_Most_Once_Message_Delivery
    if (buffered_msg)
      Message_Stats::check_received_transmission_sequence_number(msg_type, buffered_msg->transmission_serial_number, buffered_msg->sender);
# endif
    
    if (msg_type_or_encoded_acking_type != noMessage) {
      PERF_CNT(interp, count_received_intercore_messages());
      
      if (Collect_Receive_Message_Statistics)
        Message_Stats::collect_receive_msg_stats(msg_type_or_encoded_acking_type);
      break;
    }
    
    Timeout_Timer::check_all();
    
    if (!wait)
      return false;
  }
  
  Message_Or_Ack_Request* matching_msg_or_ack_request = Message_Or_Ack_Request::find(msg_type_or_encoded_acking_type);
  Message_Or_Ack_Request* matching_msg_request = msg_type == ackMessage ? NULL : matching_msg_or_ack_request;
  
  
# define MAKE_CASE(name, superclass, constructor_formals, superconstructor_actuals, constructor_body, class_body, ack_setting, safepoint_delay_setting) \
  case name: { \
    name##_class local_buf(&The_Receive_Marker); \
    name##_class* final_dest = matching_msg_request != NULL  ?  (name##_class*)matching_msg_request->buf_or_nil_for_ack  :  &local_buf; \
    /* Used to try to not copy the buffer, but if handle_me ends up receiving another msg from sender, such as doAllRootsHere, causes problems. Be simple, even if slower -- Ungar 1/10 */ \
    *final_dest = *(name##_class*)buffered_msg; \
    buffer_owner->message_queue.release_oldest_buffer(buffered_msg); \
    final_dest->handle_me_and_ack(); \
    \
    break; \
  }
  
  switch (msg_type) {
    default: { 
      fatal("bad message");
      return false;
    }

    /* all other messages */
      FOR_ALL_MESSAGES_DO(MAKE_CASE)
  }
# undef MAKE_CASE
  
  interp->safepoint_tracker->spin_if_safepoint_requested(); // xxxxxx  right place for a spin?
  
  if (matching_msg_or_ack_request != NULL)
    matching_msg_or_ack_request->is_fulfilled = true;
  
  last_msg_type = msg_type; // for debugging
  
  return true;
}


void Message_Statics::wait_for_ack(Message_Statics::messages t, int sender) {
  receive_and_handle_messages_returning_a_match(encode_msg_type_for_ack(t), NULL, sender);
}

