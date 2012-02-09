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

class ILib_Message_Queue : public Abstract_Message_Queue {
private:
   ilibBufChanSendPort    buffered_send_port;
   ilibBufChanReceivePort buffered_receive_port;
      
   void measure_point_to_point_message();
   void measure_buffered_channel();
   void measure_streaming_channel();
   void measure_raw_channel();
  
  
  
public:
  static void setup_channels();
  static void setup_buffered_channels();
  static void setup_sink();
  
  void initialize(int) {}

  void send_message(abstractMessage_class*);
  
  void buffered_send_buffer(void*, int);
  static void* buffered_receive_from_anywhere(bool wait, Logical_Core** buffer_owner, Logical_Core* const /*me*/);
  void release_oldest_buffer(void*);
  
  void measure_communication();
  
  static bool are_data_available(Logical_Core* const) {
#warning needs to be implemented, or disabled if not possible
    return false;
    //return  ilib_rawchan_available(my_raw_receive_port) > 0;
  }
  
};

# endif
