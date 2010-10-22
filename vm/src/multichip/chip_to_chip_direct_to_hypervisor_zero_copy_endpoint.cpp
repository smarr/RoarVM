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



Chip_to_Chip_Direct_to_Hypervisor_Zero_Copy_Endpoint::Chip_to_Chip_Direct_to_Hypervisor_Zero_Copy_Endpoint() 
: Abstract_Zero_Copy_Command_Queue_Endpoint() { 
}

void Chip_to_Chip_Direct_to_Hypervisor_Zero_Copy_Endpoint::prepare_to_open_all() {
  Abstract_Zero_Copy_Command_Queue_Endpoint::prepare_to_open_all();
  tile_pci_init();
  allocate_buffer_pages();
}

void Chip_to_Chip_Direct_to_Hypervisor_Zero_Copy_Endpoint::open(int i) {
  
  Abstract_Zero_Copy_Command_Queue_Endpoint::open(i);
  
  channel_id = tilepci_channel_id( channel_type(), chip_index);
  if (channel_id == TILEPCI_EINVAL) {
    lprintf("tilepci_channel_id %s failed: specified channel is not legal\n", send_or_recv());
    fatal("");
  }
  char* msg;
  int r = tilepci_open_channel(&context, channel_id);
  if (!r) return; // success
  switch (errno) {
    case TILEPCI_ECHANNEL: msg = "TILEPCI_ECHANNEL: channel_id not legal"; break;
    case ENXIO: msg = "ENXIO: channel's link is down"; break;
    case EBUSY: msg = "EBUSY: channel currently in use by the device-file-based zero copy interface"; break;
    default: msg = "unknown error"; break;
  }
  lprintf("tilepci_open_channel failed: channel_id %d, chip_index %d, result %d, errno %d: %s\n", channel_id, chip_index, r, errno, msg);
  fatal("");
}

tilepci_context_t  Chip_to_Chip_Direct_to_Hypervisor_Zero_Copy_Endpoint::context;
u_int32            Chip_to_Chip_Direct_to_Hypervisor_Zero_Copy_Endpoint::credits_per_tile;


void Chip_to_Chip_Direct_to_Hypervisor_Zero_Copy_Endpoint::send_cmd() {
  const char* msg;
  switch ( tilepci_post_cmds( &context, &command, 1) ) {
    case 0: return; // success
    case TILEPCI_EINVAL: msg = "EINVAL: a packet size was zero"; break;
    case TILEPCI_EFAULT: msg = "EFAULT: some buffer_vas was illegal"; break;
    case TILEPCI_ECHANNEL: msg = "ECHANNEL: some channel was not opened"; break;
    case TILEPCI_ECREDITS: msg = "ECREDITS: tile lacks enough credits"; break;
    default: msg = "unknown error"; break;
  }
  lprintf("tilepci_post_cmds failed, errno %d, reason %s\n", errno, msg);
  fatal("");
}

void Chip_to_Chip_Direct_to_Hypervisor_Zero_Copy_Endpoint::set_data_buf() {
  data_buf = data_buf_for(chip_index);
  
  tilepci_iomem_register(&context, data_buf, data_buf_size);
}

void Chip_to_Chip_Direct_to_Hypervisor_Zero_Copy_Endpoint::tile_pci_init() {
  credits_per_tile = TILEPCI_CMD_SLOTS / Max_Number_Of_Cores; // or TILEPCI_MAX_C2C_NCMD
  int r = ::tilepci_init(&context, device(), credits_per_tile);
  if (r) {
    char* msg;
    switch (errno) {
      default: msg = "unknown error"; break;
      case -ENOENT: msg = "ENOENT: device: does not exist"; break;
      case  TILEPCI_EBINDCPU: msg = "TILEPCI_EBINDCPU: process is not bound to a single CPU"; break;
      case -EBUSY: msg = "EBUSY: insufficient credits avaiable"; break;
      case -ENXIO: msg = "ENXIO: device exists but the PCIe link is not up"; break;
    }
    lprintf("tile_pci_init of %s for %d credits failed: %s\n", device(), credits_per_tile, msg);
    fatal("");
  }
}

void* Chip_to_Chip_Direct_to_Hypervisor_Zero_Copy_Endpoint::data_buf_for(int i) {
  return &(all_data_bufs)[(2 * i + (is_sender() ? 1 : 0)) *  data_buf_size];
}


u_int32            Chip_to_Chip_Direct_to_Hypervisor_Zero_Copy_Endpoint::total_data_buf_size = 0;
char*              Chip_to_Chip_Direct_to_Hypervisor_Zero_Copy_Endpoint::all_data_bufs = NULL;


void Chip_to_Chip_Direct_to_Hypervisor_Zero_Copy_Endpoint::allocate_buffer_pages() {
  allocate_huge_pages();
  register_data_buffer_memory();
}

void Chip_to_Chip_Direct_to_Hypervisor_Zero_Copy_Endpoint::allocate_huge_pages() {
  // total_data_buf_size = data_buf_size * TILEPCI_MAX_C2C_NCMD;
  static const int cmds_per_endpoint = 1;
  static const int endpoints_per_chip = 2;
  int num_cmds = The_Tilera_Chip_to_Chip_Message_Queue.num_chips * cmds_per_endpoint * endpoints_per_chip;
  total_data_buf_size =  data_buf_size * num_cmds;
  alloc_attr_t attr = ALLOC_INIT;
  alloc_set_huge(&attr);
  all_data_bufs = (char*)alloc_map(&attr, total_data_buf_size); // UG227 pg 85
  if (!all_data_bufs) {
    lprintf("alloc for Chip_to_Chip_Direct_to_Hypervisor_Zero_Copy_Endpoint buffer memory failed: %d bytes\n", total_data_buf_size);
    fatal("");
  }
}

void Chip_to_Chip_Direct_to_Hypervisor_Zero_Copy_Endpoint::register_data_buffer_memory() {
  int r = tilepci_iomem_register(&context, all_data_bufs, total_data_buf_size);
  char *msg;
  switch (r) {
    case 0: return; // correct!
    case TILEPCI_EFAULT: msg = "TILEPCI_EFAULT, specified address was not a huge page."; break;
    case TILEPCI_EREGISTERED: msg = "TILEPCI_EREGISTERED, specified address was already registered."; break;
    default: msg = "unknown error code";
  }
  lprintf("registration of %d bytes at 0x%x for Chip_to_Chip_Direct_to_Hypervisor_Zero_Copy_Endpoint failed: %d: %s\n", total_data_buf_size, all_data_bufs, r, msg);
  fatal("");
}

void Chip_to_Chip_Direct_to_Hypervisor_Zero_Copy_Endpoint::prepare_request(u_int32 len) {
  command.buffer = data_buf;
  command.tag = request_tag();
  command.size = len;
  command.soc = 1; // if in reset mode, must be set, ignored otherwise
  command.must_eop = 1; // not straddling data
  command.may_eop = 1;
  command.reserved = 0;
  command.channel_id = channel_id;
}

void Chip_to_Chip_Direct_to_Hypervisor_Zero_Copy_Endpoint::write_request() {
  int r = tilepci_post_cmds(&context, &command, 1);
  char* msg;
  switch (r) {
    case 0: return; // success!
    default: msg = "unknown error"; break;
    case TILEPCI_EINVAL: msg = "TILEPCI_EINVAL: a packet size was zero"; break;
    case TILEPCI_EFAULT: msg = "TILEPCI_EFAULT: a buffer was illegal"; break;
    case TILEPCI_ECHANNEL: msg = "TILEPCI_ECHANNEL: a channel_id was not open"; break;
    case TILEPCI_ECREDITS: msg = "TILEPCI_ECREDITS: this tile lacks command credits"; break;
  }
  lprintf("tilepci_post_cmds addr 0x%x, len %d failed: %d %s\n",
          command.buffer, command.size, r, msg);
  fatal("");
}

void Chip_to_Chip_Direct_to_Hypervisor_Zero_Copy_Endpoint::read_completion() {
  int r = tilepci_get_comps(&context, &completion, 1, 1);
  if (r != 1) {
    lprintf("tilepci_get_comps expected 1 got %d\n", r);
    fatal("");
  }
}

void Chip_to_Chip_Direct_to_Hypervisor_Zero_Copy_Endpoint::check_completion() {
  assert_always_eq(completion.tag, test_tag);
  assert_always(!completion.reset);
  static bool kvetched = false;
  if (!kvetched && completion.link_down) { kvetched = true; lprintf("link down on %d %s\n", chip_index, send_or_recv());  }
  // assert_always(!completion.link_down);
  assert_always_eq(completion.channel_id, command.channel_id);
}

void Chip_to_Chip_Direct_to_Hypervisor_Zero_Copy_Endpoint::move_data_to_buffer(const char* data, u_int32 len) {
  Abstract_Zero_Copy_Command_Queue_Endpoint::move_data_to_buffer(data, len);
  OS_Interface::mem_flush(data_buf, len);
}

int Chip_to_Chip_Direct_to_Hypervisor_Zero_Copy_Endpoint::move_data_from_buffer(char* data) {
  OS_Interface::invalidate_mem( command.buffer, completion.size);
  memcpy(data, command.buffer, completion.size);
  return completion.size;
}


# endif // Multiple_Tileras
