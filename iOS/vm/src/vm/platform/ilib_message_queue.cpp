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

# if On_Tilera

#include "headers.h"


void ILib_Message_Queue::setup_channels() {
  setup_buffered_channels();
}

void ILib_Message_Queue::setup_buffered_channels() {
  if (Logical_Core::running_on_main())
    FOR_ALL_RANKS(sender) {
      FOR_ALL_RANKS(receiver) {
        if (sender != receiver)
          OS_Interface::abort_if_error("buffchan_connect",
                         ilib_bufchan_connect(ILIB_GROUP_SIBLINGS, sender, receiver,
                                              receiver, sender));
      }
    }
  int buf_size =  ilib_bufchan_calc_buffer_size(Number_Of_Channel_Buffers,
                                                Message_Statics::max_message_size());
  ilibRequest send_requests[Max_Number_Of_Cores];
  FOR_ALL_OTHER_RANKS(other_end) {
    Logical_Core *other = &logical_cores[other_end];
    OS_Interface::abort_if_error("open buffered sender",
                   ilib_bufchan_start_open_sender( other_end, &other->message_queue.buffered_send_port, &send_requests[other_end]));
  }
  FOR_ALL_OTHER_RANKS(other_end) {
    Logical_Core *other = &logical_cores[other_end];
    char* buffer = (char*)malloc(buf_size);
    OS_Interface::abort_if_error("open buffered receiver",
                   ilib_bufchan_open_receiver(other_end, buffer, buf_size, &other->message_queue.buffered_receive_port));
  }
  FOR_ALL_OTHER_RANKS(other_end) {
    ilibStatus status;
    OS_Interface::abort_if_error("wait for buffered open", ilib_wait( &send_requests[other_end], &status));
  }
}


void ILib_Message_Queue::buffered_send_buffer(void* p, int sz) {
  OS_Interface::abort_if_error("buffered send", ilib_bufchan_send(buffered_send_port, p, sz));
}

void* ILib_Message_Queue::buffered_receive_from_anywhere(bool wait, Logical_Core** buffer_owner, Logical_Core* const /*me*/) {
  do {
    FOR_ALL_OTHER_RANKS(r) {
      size_t sz;
      void* p = ilib_bufchan_receive_noblock( logical_cores[r].message_queue.buffered_receive_port, &sz );
      if (p != NULL) {
        *buffer_owner = &logical_cores[r];
        return p;
      }
    }
  }
  while (wait);
  *buffer_owner = NULL;
  return NULL;
}

void ILib_Message_Queue::release_oldest_buffer(void*) {
  ilib_bufchan_release_one(buffered_receive_port);
}


#warning The following code is not maintained, needs to be tested whether it \
         still works (Stefan 2010-08-03)

void ILib_Message_Queue::measure_communication() {
  if (Logical_Core::my_rank() >= 2) {
    rvm_exit();
    return;
  }
  if (Logical_Core::num_cores < 2)
    fatal("not enough cores to measure it");
  measure_point_to_point_message();
  measure_buffered_channel();
  measure_streaming_channel();
  measure_raw_channel();
  The_Measurements.print();
  rvm_exit();
}


static const int msg_size_in_bytes = 40;
static char msg_buf[msg_size_in_bytes];

static inline void sync_for_measuring() {
  if (Print_Barriers) lprintf("Barrier in sync_for_measuring()");
  OS_Interface::abort_if_error("barrier", ilib_msg_barrier(ILIB_GROUP_SIBLINGS));
  if (Print_Barriers) lprintf("Barrier done\n");
}

void ILib_Message_Queue::measure_point_to_point_message() {
  for (int i = 0;  i < 1000; ++i) {
    sync_for_measuring();
    int err;
    if (Logical_Core::my_rank() == 0) {
      MEASURE(send_point_to_point_message, err, err = ilib_msg_send(ILIB_GROUP_SIBLINGS, 1, 0, msg_buf, sizeof(msg_buf)));
    }
    else {
      ilibStatus status;
      MEASURE(receive_point_to_point_message, err,  err = ilib_msg_receive(ILIB_GROUP_SIBLINGS, 0, 0, msg_buf, sizeof(msg_buf), &status));
    }
    OS_Interface::abort_if_error("measure_point_to_point_message send/rcv", err);
  }
}

void ILib_Message_Queue::measure_buffered_channel() {
  if (Logical_Core::my_rank() == 0)
    OS_Interface::abort_if_error("measure_buffered_channel connect", ilib_bufchan_connect(ILIB_GROUP_SIBLINGS, 0, 0, 1, 0));
  
  ilibBufChanSendPort send_port;
  ilibBufChanReceivePort receive_port;
  char* rcv_buf;
  if (Logical_Core::my_rank() == 0)   OS_Interface::abort_if_error("measure_buffered_channel open sender",  ilib_bufchan_open_sender(0, &send_port) );
  else {
    int sz = ilib_bufchan_calc_buffer_size(2, sizeof(msg_buf));
    rcv_buf = (char*)malloc(sz);
    OS_Interface::abort_if_error("measure_buffered_channel open receiver",  ilib_bufchan_open_receiver(0, rcv_buf, sz, &receive_port ));
  }
  
  for (int i = 0;  i < 1000; ++i) {
    sync_for_measuring();
    int err;
    if (Logical_Core::my_rank() == 0) {
      MEASURE(send_bufchan_message, err, err = ilib_bufchan_send(send_port, msg_buf, sizeof(msg_buf)));
      OS_Interface::abort_if_error("measure_buffered_channel send", err);
    }
    else {
      int err;
      __attribute__((unused)) void* str;
      MEASURE(receive_bufchan_message, str,  str = ilib_bufchan_receive(receive_port, (size_t*)&err));
      OS_Interface::abort_if_error("measure_buffered_channel receive", err);
      MEASURE(release_bufchan_message, err, err = (ilib_bufchan_release_one(receive_port), 0));
    }
  }
  if (Logical_Core::my_rank() == 1)
    free(rcv_buf);
}


static u_int32 wb[10];



void ILib_Message_Queue::measure_streaming_channel() {
  if (Logical_Core::my_rank() == 0)
    OS_Interface::abort_if_error("measure_streaming_channel connect", ilib_strchan_connect(ILIB_GROUP_SIBLINGS, 0, 0, 1, 0));
  
  ILIB_STR_SEND_PORT(send_port, 1);
  ILIB_STR_RECEIVE_PORT(receive_port, 1);
  
  if (Logical_Core::my_rank() == 0)   OS_Interface::abort_if_error("measure_streaming_channel open sender",  ilib_strchan_open_sender(0, send_port) );
  else             OS_Interface::abort_if_error("measure_streaming_channel open receiver",  ilib_strchan_open_receiver(0, receive_port ));
  
  for (int i = 0;  i < 1000; ++i) {
    __attribute__((unused))  int err;
    if (Logical_Core::my_rank() == 0) {
      sync_for_measuring();
# define P(x) fprintf(stderr, "%s\n", #x);
      MEASURE(send_strchan, err, {
        ilib_strchan_send_10(send_port, wb[0], wb[1], wb[2], wb[3], wb[4], wb[5], wb[6], wb[7], wb[8], wb[9]);
        err = 0; });
    }
    else {
      sync_for_measuring();
      MEASURE( receive_strchan, err, {
        ilib_strchan_receive_10(receive_port, wb[0], wb[1], wb[2], wb[3], wb[4], wb[5], wb[6], wb[7], wb[8], wb[9]);
        err = 0;
      });
    }
  }
}

void ILib_Message_Queue::measure_raw_channel() {
  if (Logical_Core::my_rank() == 0)
    OS_Interface::abort_if_error("measure_raw_channel connect", ilib_rawchan_connect(ILIB_GROUP_SIBLINGS, 0, 0, 1, 0));
  
  ILIB_RAW_SEND_PORT(raw_send_port, 2);
  ILIB_RAW_RECEIVE_PORT(receive_port, 2);
  
  if (Logical_Core::my_rank() == 0)   OS_Interface::abort_if_error("measure_raw_channel open sender",  ilib_rawchan_open_sender(0, raw_send_port) );
  else             OS_Interface::abort_if_error("measure_raw_channel open receiver",  ilib_rawchan_open_receiver(0, receive_port ));
  
  for (int i = 0;  i < 1000; ++i) {
    sync_for_measuring();
    __attribute__((unused)) int err;
    if (Logical_Core::my_rank() == 0) {
      MEASURE(send_rawchan, err, {
        ilib_rawchan_send_10(raw_send_port, wb[0], wb[1], wb[2], wb[3], wb[4], wb[5], wb[6], wb[7], wb[8], wb[9]);
        err = 0;
      });
    }
    else {
      MEASURE( receive_rawchan, err, {
        ilib_rawchan_receive(receive_port);
        ilib_rawchan_receive(receive_port);
        ilib_rawchan_receive(receive_port);
        ilib_rawchan_receive(receive_port);
        ilib_rawchan_receive(receive_port);
        ilib_rawchan_receive(receive_port);
        ilib_rawchan_receive(receive_port);
        ilib_rawchan_receive(receive_port);
        ilib_rawchan_receive(receive_port);
        ilib_rawchan_receive(receive_port);
        err = 0;
      });
    }
  }
}







# include <signal.h>


void ILib_Message_Queue::send_message(abstractMessage_class* msg) {
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
  
  if (verbose) lprintf("send_message about to send header %d, three words: 0x%x, 0x%x, 0x%x\n",
                       msg->header, ((int*)msg)[0], ((int*)msg)[1], ((int*)msg)[2]);
  
  
  if (verbose) lprintf("send_message about to send %d bytes, three words: 0x%x, 0x%x, 0x%x\n",
                       Logical_Core::my_rank(), msg->size_for_transmission_and_copying(), ((int*)msg)[0], ((int*)msg)[1], ((int*)msg)[2]);
  
  buffered_send_buffer(msg, msg->size_for_transmission_and_copying());
  
  if (verbose) lprintf( "send_message sent\n");
}




# endif // On_Tilera
