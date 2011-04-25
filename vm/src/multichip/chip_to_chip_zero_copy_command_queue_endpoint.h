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


class Chip_to_Chip_Zero_Copy_Command_Queue_Endpoint: public Abstract_Zero_Copy_Command_Queue_Endpoint {
protected:
  
  int fd;
  char file_name[BUFSIZ];
  
  tilepci_xfer_req_t  request;  
  tilepci_xfer_comp_t completion;
  
  
public:
  Chip_to_Chip_Zero_Copy_Command_Queue_Endpoint();
  
  void open(int i);
  
  
protected:
  void set_file_name() { sprintf(file_name, "/dev/hostpci/c2c_%s/%d", send_or_recv(), chip_index); }
  void get_read_error_string(int err, char* buf);
  void get_write_error_string(int err, char* buf); 
  static void get_flag_string(int flags, char* buf);
  void flags_should_be(int desired, char* msg);
  void io_failure(bool);
  
  void prepare_request(u_int32);
  virtual int request_flags() = 0;
  virtual int request_cookie() = 0;
  
  void write_request();
  void read_completion();
  
  void check_completion();
  int move_data_from_buffer(char* data);  
  void set_data_buf();
  static const int test_cookie = 17;
};


# endif // Multiple_Tileras
