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


# if !On_Tilera

class Shared_Memory_Message_Queue : public Abstract_Message_Queue {
protected:
  #if Use_BufferedChannelDebug
    BufferedChannelDebug buffered_channel;
    # define CHANNEL_BUFFER_SIZE 10
    void* buffer_for_channel[CHANNEL_BUFFER_SIZE];
  #else
    BufferedChannel      buffered_channel;
  #endif

public:
  void* operator new(size_t sz);  // Needs to be in shared space for process version
  
  Shared_Memory_Message_Queue() :
    #if Use_BufferedChannelDebug
      buffered_channel(BufferedChannelDebug(&buffer_for_channel, CHANNEL_BUFFER_SIZE)) {}
    #else
      buffered_channel(BufferedChannel(Number_Of_Channel_Buffers, Message_Statics::max_message_size())) {}
    #endif
  
  
  void send_message(abstractMessage_class*);
  
  void buffered_send_buffer(void*, int);
  static void* buffered_receive_from_anywhere(bool wait, Logical_Core** buffer_owner, Logical_Core* const receiver);
  void release_oldest_buffer(void*);
  
  
  static bool are_data_available(Logical_Core* const /* receiver */) {
    // TODO STEFAN: was never implemented for buffered channels, 
    //    there is no api on tilera for that, on x86 I should fix my queue,
    //    or measure whether the debug version would slow it down,
    //    if it would use hasData() to implement this here.
    return false; //buffered_channel.hasData();
  }
  
};

# endif
