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


# if Multiple_Tileras

#include "headers.h"

Tilera_Chip_to_Chip_Message_Queue The_Tilera_Chip_to_Chip_Message_Queue;


// Use pagesize for now; works for both ways
u_int32 Abstract_Zero_Copy_Command_Queue_Endpoint::data_buf_size = max(getpagesize(), __ILIB_L2_CACHELINE_SIZE);


# define FOR_ALL_CHIPS_BUT_ME(index) \
for (int index = 0;  (index == my_chip_index ? ++index : 0),  index < num_chips;  ++index)

void connect_to_other_chips() { The_Tilera_Chip_to_Chip_Message_Queue.init(); }

void send_test_message() {
  The_Tilera_Chip_to_Chip_Message_Queue.send_test_message();
  The_Tilera_Chip_to_Chip_Message_Queue.send_test_message_asap();
}


void Tilera_Chip_to_Chip_Message_Queue::init() {
  max_request_count = TILEPCI_MAX_C2C_NCMD;
  num_chips = The_Squeak_Interpreter()->num_chips();
  my_chip_index = info.Host_Link_Index;
  lprintf("my_chip_index %d, num_chips %d, max_chip_count %d\n", num_chips, my_chip_index, max_chip_count);
  assert_always(num_chips <= max_chip_count  &&  my_chip_index < num_chips);
  
  info.print();
  open_channels();
}


void Tilera_Chip_to_Chip_Message_Queue::open_channels() {
  senders[0].prepare_to_open_all();
  FOR_ALL_CHIPS_BUT_ME(i) {
    senders  [i].open(i);
    receivers[i].open(i);
  }
}

void Tilera_Chip_to_Chip_Message_Queue::send(u_int32 chip_index, const char* data, u_int32 len) {
  assert_always(chip_index != my_chip_index);
  assert_always(chip_index < num_chips);
  senders[chip_index].send(data, len);
}

int Tilera_Chip_to_Chip_Message_Queue::recv(u_int32 chip_index, char* data, u_int32 len) {
  assert_always(chip_index < num_chips);
  assert_always(chip_index != my_chip_index);
  return receivers[chip_index].recv(data, len);
}



void Tilera_Chip_to_Chip_Message_Queue::send_test_message() {
  int msg1, msg2;
  char buf[BUFSIZ];
  static const bool verbose = false;
  static const int N = 1000;
  switch (my_chip_index) {
    default: break;
    case 0: {
       lprintf("sending test message\n");
       int64 start = OS_Interface::get_cycle_count();
        for (int i = 1;  i <= N;  ++i) {
          msg1 = i * 2;
          send(1, (char*)&msg1, sizeof(msg1));
          if (verbose)  lprintf("sent\n");
          int n = recv(1, buf, sizeof(buf));
          assert_always_eq(n, sizeof(msg2));
          assert_always_eq(i * 2  +  1, *(int*)buf);
          if (verbose)  lprintf("received\n");
        }
        int64 end = OS_Interface::get_cycle_count();
        lprintf("Success!! Send & Received %s in %lld cycles/rt\n", buf, (end - start) / (int64)N);
      }
      break;
    case 1: 
      lprintf("relaying test message\n");
      for (int i = 1;  i <= N;  ++i) {
        int n = recv(0, buf, sizeof(buf));
        if (verbose)  lprintf("received\n");
        assert_always_eq(n, sizeof(msg1));
        assert_always_eq(i * 2, *(int*)buf);
        msg2 = i * 2  +  1;
        send(0, (char*)&msg2, sizeof(msg2));
        if (verbose)  lprintf("sent\n");
      }
      lprintf("Success!! Received %s and sent\n", buf);
      break;
  }
}




void Tilera_Chip_to_Chip_Message_Queue::send_test_message_asap() {
  int msg1, msg2;
  char buf[BUFSIZ];
  static const bool verbose = false;
  static const int N = 1000;
  int other_index = 1 - my_chip_index;
  static Chip_to_Chip_Sender sender = senders[other_index];
  static Chip_to_Chip_Receiver receiver = receivers[other_index];
  tilepci_context_t* context = &sender.context;
  char* s_data_buf = (char*)sender.data_buf;
  char* r_data_buf = (char*)receiver.data_buf;
  
  static tilepci_cmd_t  s_command;
  s_command.buffer = s_data_buf;
  s_command.size = 4;
  s_command.tag = 17;
  s_command.soc = 1;
  s_command.must_eop = 1;
  s_command.may_eop = 1;
  s_command.reserved = 0;
  s_command.channel_id = sender.channel_id;

  
  static tilepci_cmd_t  r_command;
  r_command.buffer = r_data_buf;
  r_command.size = getpagesize();
  r_command.tag = 19;
  r_command.soc = 1;
  r_command.must_eop = 1;
  r_command.may_eop = 1;
  r_command.reserved = 0;
  r_command.channel_id = receiver.channel_id;

  
  static tilepci_comp_t completion;
  
  1 + 2;
  
  switch (my_chip_index) {
    default: break;
    case 0: {
      lprintf("sending test message\n");
      int64 start = OS_Interface::get_cycle_count();
 
      for (int i = 1;  i <= N;  ++i) {
        msg1 = i * 2;
        
         *(int*)s_data_buf = msg1;
        OS_Interface::mem_flush(s_data_buf, 4);
        int r;
        r = tilepci_post_cmds(context, &s_command, 1);
        if (r) fatal("tilepci_post_cmds");

        r = tilepci_get_comps(context, &completion, 1, 1);
        if (r != 1) fatal("tilepci_get_comps");
              
              
        
        r = tilepci_post_cmds(context, &r_command, 1);
        if (r) fatal("tilepci_post_cmds");
        
        r = tilepci_get_comps(context, &completion, 1, 1);
        if (r != 1) fatal("tilepci_get_comps");

        OS_Interface::invalidate_mem( r_data_buf, 4);
        msg1 = *(int*)r_data_buf;
        assert_always_eq(i * 2  +  1, msg1);
      }
      int64 end = OS_Interface::get_cycle_count();
      lprintf("Success!! Send & Received ASAP %s in %lld cycles/rt\n", buf, (end - start) / (int64)N);
    }
      break;
      
    case 1: 
      lprintf("relaying test message\n");
      for (int i = 1;  i <= N;  ++i) {
         
        int r;
        r = tilepci_post_cmds(context, &r_command, 1);
        if (r) fatal("tilepci_post_cmds");
        
        r = tilepci_get_comps(context, &completion, 1, 1);
        if (r != 1) fatal("tilepci_get_comps");
        
        OS_Interface::invalidate_mem( r_data_buf, 4);
        msg2 = *(int*)r_data_buf;
        assert_always_eq(i * 2 , msg2);
        
        
        
        msg2 = i * 2 + 1;
        
        *(int*)s_data_buf = msg2;
        OS_Interface::mem_flush(s_data_buf, 4);
        r = tilepci_post_cmds(context, &s_command, 1);
        if (r) fatal("tilepci_post_cmds");
        
        r = tilepci_get_comps(context, &completion, 1, 1);
        if (r != 1) fatal("tilepci_get_comps");
      }
      lprintf("Success!! Received %s and sent\n", buf);
      break;
  }
}




# endif

