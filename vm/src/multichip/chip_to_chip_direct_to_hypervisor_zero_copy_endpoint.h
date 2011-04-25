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


class Chip_to_Chip_Direct_to_Hypervisor_Zero_Copy_Endpoint: public Abstract_Zero_Copy_Command_Queue_Endpoint {
  friend class Tilera_Chip_to_Chip_Message_Queue; // for asap benchmark


public:
  Chip_to_Chip_Direct_to_Hypervisor_Zero_Copy_Endpoint();
  
  static void prepare_to_open_all();
  void open(int);
  
  
protected:
  static const char* device() { return "/dev/hostpci/hv_direct_zc"; }
  static tilepci_context_t context;
  static u_int32 credits_per_tile;
  int channel_id;
  tilepci_cmd_t  command;
  tilepci_comp_t completion;
  
  virtual tilepci_channel_type_t channel_type() = 0;
  void send_cmd();
  
  void set_data_buf();
  static void tile_pci_init();
  
  void*   data_buf_for(int);
  static u_int32 total_data_buf_size;
  static char* all_data_bufs;
  
  
  static void allocate_buffer_pages();
  static void allocate_huge_pages();
  static void register_data_buffer_memory();
  void prepare_request(u_int32);
  virtual int request_tag() = 0;
  void write_request();
  void read_completion();
  void check_completion();
  void move_data_to_buffer(const char* data, u_int32 len);
  int move_data_from_buffer(char* data);

  static const int test_tag = 29;
};


# endif // Multiple_Tileras
