/******************************************************************************
 *  Copyright (c) 2008 - 2010 IBM Corporation and others.
 *  All rights reserved. This program and the accompanying materials
 *  are made available under the terms of the Eclipse Public License v1.0
 *  which accompanies this distribution, and is available at
 *  http://www.eclipse.org/legal/epl-v10.html
 * 
 *  Contributors:
 *    Stefan Marr, Vrije Universiteit Brussel - Initial Implementation
 *
 *  
 *  This is the debug version of our buffered channel implementation.
 *  While the buffered_channel.h implementation was build from scratch, to
 *  be highly optimized, it might be buggy.
 *  This version is implemented using the simples possible strategy and std::queue
 *  to ensure that is correct. But it might be less performant.
 *
 ******************************************************************************/


#include <stdint.h>
#include <vector>

using namespace std;

class BufferedChannelDebug {
private:
  Circular_Buffer<void*> channel;
  OS_Interface::Mutex lock;

public:
  void* operator new(size_t /* sz */ ) { 
    fatal("Make sure new is not called, "
          "and the objects resides in the expected memory");
  }
  
  BufferedChannelDebug(void* channel_buffer, size_t channel_size)
  : channel(Circular_Buffer<void*>(channel_buffer, channel_size)) { //vector<void*, shared_malloc_allocator<void*> >(10, NULL, shared_malloc_allocator<void*>())
    if (Using_Processes)
      OS_Interface::mutex_init_for_cross_process_use(&lock);
    else
      OS_Interface::mutex_init(&lock);
  }

  /**
   * TODO: This is inherently unsafe!
   * I will not care about it at the moment, but this behavior is undefined.
   * Especially since I cannot reliably destroy a locked mutex.
   */
  ~BufferedChannelDebug() {
    OS_Interface::mutex_lock(&lock);
    
    while (!channel.is_empty()) {
      free(channel.dequeue());
    }
    
    OS_Interface::mutex_unlock(&lock);
    
    OS_Interface::mutex_destruct(&lock);
  }

  void send(const void* data, size_t size);
  
  void* receive(size_t&) {
    OS_Interface::mutex_lock(&lock);
    void* const result = channel.dequeue();
    OS_Interface::mutex_unlock(&lock);
    
    return result;
  }
  
  void releaseOldest(void* const buffer) const;

  bool hasData() {
    return !channel.is_empty();
  }

};

