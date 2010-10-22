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

Chip_to_Chip_Zero_Copy_Command_Queue_Endpoint::Chip_to_Chip_Zero_Copy_Command_Queue_Endpoint() 
: Abstract_Zero_Copy_Command_Queue_Endpoint() { 
  chip_index = -1; 
  fd = -1; 
  file_name[0] = '\0'; 
}


void Chip_to_Chip_Zero_Copy_Command_Queue_Endpoint::open(int i) {
  Abstract_Zero_Copy_Command_Queue_Endpoint::open(i);
  
  set_file_name(); 
  fd = ::open(file_name, O_RDWR);
  if (fd >= 0) lprintf("%s open on %d: %d\n", file_name, i, fd);
  else {perror(file_name); fatal(""); }
}

void Chip_to_Chip_Zero_Copy_Command_Queue_Endpoint::get_read_error_string(int err, char* buf) {
  const char* msg;
  switch (err) {
    case EINVAL: msg = "The read size was not a multiple of sizeof(tilepci_xfer_comp_t).";  
      break;
    case ENXIO: msg = "Connection is down. This can occur if the TILExpress card is reset while the zero-copy command queue file handle is open.";  
      break;
    case EAGAIN: msg = "The operation would block. This error is only returned if the file handle has the O_NONBLOCK flag set."; 
      break;
    case EFAULT: msg = "The completion array is not in accessible address space.";  
      break;
    case EINTR: msg = "The request was interrupted by a signal before any commands were posted.";  
      break;
    default:
      sprintf(buf, "Unknown error: %d", err);
      return;
  }
  strcpy(buf, msg);
}

void Chip_to_Chip_Zero_Copy_Command_Queue_Endpoint::get_write_error_string(int err, char* buf) {
  char* msg;
  switch (err) {
    case EINVAL: msg = "The write size was not a multiple of sizeof(tilepci_xfer_req_t), or a buffer specified by one of the requests was not physically contiguous.";
      break;
    case ENOBUFS: msg = "The number of commands set by the TILEPCI_IOC_SET_NCMD ioctl() is too small to ever transmit this many requests.";
      break;
    case ENXIO: msg = "Connection is down. This can occur if the TILExpress card is reset while the zero-copy command queue file handle is open";
      break;
    case EAGAIN: msg = "The operation would block. This error is only returned if the file handle has the O_NONBLOCK flag set.";
      break;
    case EFAULT: msg = "Either the command array passed to write() or the data buffer specified by one of the written commands were not in accessible address space.";
      break;
    case EINTR: msg = "The request was interrupted by a signal before any commands were posted.";
    default:
      sprintf(buf, "Unknown error: %d", err);
      return;
  }
  strcpy(buf, msg);
}

void Chip_to_Chip_Zero_Copy_Command_Queue_Endpoint::get_flag_string(int flags, char* buf) {
  if (flags == 0) { strcpy(buf, "<none>"); return; }
  
  int f = flags;
  buf[0] = '\0';
# define do_flag(flag_name) if (f & flag_name) {f &= ~flag_name; strcat(buf, #flag_name  " "); } else
  do_flag(TILEPCI_SEND_EOP);
  do_flag(TILEPCI_RCV_MAY_EOP);
  do_flag(TILEPCI_RCV_MUST_EOP);
  do_flag(TILEPCI_CPL_EOP);
  do_flag(TILEPCI_CPL_OVERFLOW);
  do_flag(TILEPCI_CPL_RESET);
  do_flag(TILEPCI_CPL_LINK_DOWN);
  char buf2[BUFSIZ];
  strcpy(buf2, buf);
  if (f) sprintf(buf, "%s <unknown: 0x%x>", buf2, f);
}

void Chip_to_Chip_Zero_Copy_Command_Queue_Endpoint::flags_should_be(int desired, char* msg) {
  if (completion.flags == desired) return;
  char fb[BUFSIZ];
  get_flag_string(completion.flags, fb);
  char fb2[BUFSIZ];
  get_flag_string(desired, fb2);
  char buf[BUFSIZ];
  sprintf(buf, "%s comp flags %s(0x%x) should be %s(0x%x)\n", msg, fb, completion.flags, fb2, desired);
  lprintf(buf);
  fatal("");
}

void Chip_to_Chip_Zero_Copy_Command_Queue_Endpoint::io_failure(bool is_write) {
  int err = errno;
  lprintf("%s to zero-copy queue from %d to %s failed: \n", 
          (is_write ? "write" : "read"), The_Tilera_Chip_to_Chip_Message_Queue.my_chip_index, file_name);
  char msg[BUFSIZ];
  if (is_write) get_write_error_string(err, msg);
  else          get_read_error_string( err, msg);
  lprintf("error: %s\n", msg);
  fatal("");
}

void Chip_to_Chip_Zero_Copy_Command_Queue_Endpoint::prepare_request(u_int32 len) {
  request.addr = data_buf;;
  request.len = len;
  request.flags = request_flags();
  request.cookie = request_cookie();
}

void Chip_to_Chip_Zero_Copy_Command_Queue_Endpoint::write_request() {
  int r = write(fd, &request, sizeof(request));  
  if (r != sizeof(request)) io_failure(true);
}

void Chip_to_Chip_Zero_Copy_Command_Queue_Endpoint::read_completion() {
  int r = read( fd, &completion, sizeof(completion));
  if (r != sizeof(completion)) io_failure(false);
}

void Chip_to_Chip_Zero_Copy_Command_Queue_Endpoint::check_completion() {
  assert_always_eq(completion.addr,   request.addr);
  assert_always_eq(completion.cookie, test_cookie);
}

int Chip_to_Chip_Zero_Copy_Command_Queue_Endpoint::move_data_from_buffer(char* data) {
  memcpy(data, request.addr, completion.len);
  return completion.len;
}

void Chip_to_Chip_Zero_Copy_Command_Queue_Endpoint::set_data_buf() {
  lprintf("FYI, data buf size = %d\n", data_buf_size);
  data_buf = OS_Interface::rvm_memalign(getpagesize(), data_buf_size); 
  assert_always(data_buf);
}


# endif // Multiple_Tileras
