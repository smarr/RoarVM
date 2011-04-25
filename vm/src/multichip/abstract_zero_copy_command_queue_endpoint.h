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


class Abstract_Zero_Copy_Command_Queue_Endpoint {
  
public:
  Abstract_Zero_Copy_Command_Queue_Endpoint() { chip_index = -1;  data_buf = NULL;  }
  static void prepare_to_open_all() {}
  void open(int);
  
  virtual bool is_sender() = 0;
  bool is_receiver() { return !is_sender(); }
  const char* send_or_recv() { return is_sender() ? "send" : "recv"; }
  
  void send(const char*, u_int32);
  int  recv(      char*, u_int32);
  
  
protected:
  int chip_index;
  void* data_buf;
  static u_int32 data_buf_size;
  
  virtual void set_data_buf() = 0;
  void check_send_constraints(u_int32);
  void check_recv_constraints(u_int32);
  void check_data_size(u_int32);
  
  virtual void write_request() = 0;
  virtual void read_completion() = 0;
  virtual void  check_completion() = 0;
  virtual int move_data_from_buffer(char* data) = 0;
  virtual void move_data_to_buffer(const char* data, u_int32 len);
  virtual void prepare_request(u_int32) = 0;
  
};




# endif // Multiple_Tileras
