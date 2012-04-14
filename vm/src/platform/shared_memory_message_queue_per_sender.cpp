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

# if Use_PerSender_Message_Queue

void Shared_Memory_Message_Queue_Per_Sender::buffered_send_buffer(void* p, int size) {
  buffered_channels[Logical_Core::my_rank()].channel.send(p, size);
}


void* Shared_Memory_Message_Queue_Per_Sender::buffered_receive_from_anywhere(bool wait, Logical_Core** buffer_owner, Logical_Core* const me) {
  do {
    size_t size;
    FOR_ALL_OTHER_RANKS(i) {
      if (me->message_queue.buffered_channels[i].channel.hasData()) {
        *buffer_owner = me;
        return (void*)me->message_queue.buffered_channels[i].channel.receive(size);
      }
    }
    OS_Interface::mem_fence();
  }
  while (wait);
  *buffer_owner = NULL;
  return NULL;
}


void Shared_Memory_Message_Queue_Per_Sender::release_oldest_buffer(void* buffer_to_be_released_for_debugging) {
  Memory_Semantics::shared_free(buffer_to_be_released_for_debugging);
}



# include <signal.h>


void Shared_Memory_Message_Queue_Per_Sender::send_message(abstractMessage_class* msg) {
#warning STEFAN: needs to be refactored. Shared_Memory_Message_Queue_Per_Sender and Shared_Memory_Message_Queue have identical implementations.
  
  Message_Stats::collect_send_msg_stats(msg->header);
  bool verbose = false;
  switch(msg->header) {
      // case Message_Statics::requestSafepointMessage:
      // case Message_Statics::noMessage:
    case Message_Statics::addObjectFromSnapshotMessage:
    case Message_Statics::addObjectFromSnapshotResponse:
    case Message_Statics::broadcastInterpreterDatumMessage:
      verbose = false;
  }
  
  if (verbose) lprintf( "send_message about to send header %d, three words: 0x%x, 0x%x, 0x%x\n",
                       msg->header, ((int*)msg)[0], ((int*)msg)[1], ((int*)msg)[2]);
  
  
  if (verbose) lprintf( "send_message about to send to %d, size: %d bytes, three words: 0x%x, 0x%x, 0x%x\n",
                       cpu_core_my_rank(), msg->size_for_transmission_and_copying(), ((int*)msg)[0], ((int*)msg)[1], ((int*)msg)[2]);
  
  buffered_send_buffer(msg, msg->size_for_transmission_and_copying());
  
  if (verbose) lprintf( "send_message sent\n");
}

# endif // !Use_PerSender_Message_Queue
