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

# define Use_File_Based_API 0

# if Use_File_Based_API
typedef Chip_to_Chip_Zero_Copy_Command_Sender   Chip_to_Chip_Sender;
typedef Chip_to_Chip_Zero_Copy_Command_Receiver Chip_to_Chip_Receiver;
# else
typedef Chip_to_Chip_Direct_to_Hypervisor_Zero_Copy_Sender   Chip_to_Chip_Sender;
typedef Chip_to_Chip_Direct_to_Hypervisor_Zero_Copy_Receiver Chip_to_Chip_Receiver;
# endif


class Tilera_Chip_to_Chip_Message_Queue {

public:
  static const int max_chip_count = 16;
  int num_chips;
  int my_chip_index;
  u_int32 max_request_count;
  
private:
  Chip_to_Chip_Sender   senders  [max_chip_count];
  Chip_to_Chip_Receiver receivers[max_chip_count];
  
  Host_PCI_Info info;

  
  void open_channels();
  
  void get_write_error_string(int, char*);
  void get_read_error_string(int, char*);
  
  public:
  void init();
  void send(u_int32 chip_index, const char* data, u_int32 len);
  int recv(u_int32 chip_index, char* data, u_int32 len);
  
  void send_test_message();
  void send_test_message_asap();
};

extern Tilera_Chip_to_Chip_Message_Queue The_Tilera_Chip_to_Chip_Message_Queue;

extern "C" { void connect_to_other_chips(); void send_test_message(); }

# endif
