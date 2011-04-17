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



void Abstract_Zero_Copy_Command_Queue_Endpoint::open(int i) {
  chip_index = i;
  set_data_buf();
}


void Abstract_Zero_Copy_Command_Queue_Endpoint::send(const char* data, u_int32 len) {
  check_send_constraints(len);
  move_data_to_buffer(data, len);
  prepare_request(len);
  write_request();
  read_completion();
  check_completion();
}



int Abstract_Zero_Copy_Command_Queue_Endpoint::recv(char* data, u_int32 len) {
  check_recv_constraints(len);
  prepare_request(len);
  write_request();
  read_completion();
  check_completion();
  return move_data_from_buffer(data);
}



void Abstract_Zero_Copy_Command_Queue_Endpoint::check_data_size(u_int32 len) {
  static const int max_size = 64 * 1024;
  assert_always(len <= max_size);
  assert_always(len <= data_buf_size);
}
void Abstract_Zero_Copy_Command_Queue_Endpoint::check_send_constraints(u_int32 len) {
  assert_always(is_sender());
  check_data_size(len);
}
void Abstract_Zero_Copy_Command_Queue_Endpoint::check_recv_constraints(u_int32 len) {
  assert_always(!is_sender());
  check_data_size(len);
}


void Abstract_Zero_Copy_Command_Queue_Endpoint::move_data_to_buffer(const char* data, u_int32 len) {
  memcpy(data_buf, data, len); 
}


# endif // Multiple_Tileras
