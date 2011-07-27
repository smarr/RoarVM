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

class Shared_Memory_Message_Queue_Per_Sender : public Abstract_Message_Queue {
protected:
  #if Use_BufferedChannelDebug
  struct {
    BufferedChannelDebug channel;
    // TODO: STEFAN: this is ad-hoc but does the job for the moment, should be changed
    # define INTEL_CACHELINE_SIZE 64
    # define SIZE_TO_FIT_IN_CHANNEL (INTEL_CACHELINE_SIZE * 2)
    char cacheline_alignment[SIZE_TO_FIT_IN_CHANNEL - sizeof(BufferedChannelDebug)];
  } buffered_channels[Max_Number_Of_Cores];
  #else
    BufferedChannel      buffered_channel;
  #endif

public:
  void* operator new(size_t sz);  // Needs to be in shared space for process version
  
  Shared_Memory_Message_Queue_Per_Sender()
    #if Use_BufferedChannelDebug
      {}
    #else
      : buffered_channel(BufferedChannel(Number_Of_Channel_Buffers, Message_Statics::max_message_size())) {}
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
