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

using namespace std;

class BufferedChannelDebug {
private:
  queue<void*> channel;
  OS_Interface::Mutex lock;

public:
  BufferedChannelDebug() {
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
    
    while (!channel.empty()) {
      free(channel.front());
      channel.pop();
    }
    
    OS_Interface::mutex_unlock(&lock);
    
    OS_Interface::mutex_destruct(&lock);
  }

  void send(const void* data, size_t size) {
    void* const buffer = OS_Interface::rvm_malloc_shared(size);
    memcpy(buffer, data, size);
  
    OS_Interface::mutex_lock(&lock);
    channel.push(buffer);
    OS_Interface::mutex_unlock(&lock);
  }
  
  void* receive(size_t&) {
    OS_Interface::mutex_lock(&lock);
    void* const result = channel.front();
    channel.pop();
    OS_Interface::mutex_unlock(&lock);
    
    return result;
  }
  
  void releaseOldest(void* const buffer) const {
    OS_Interface::rvm_free_shared(buffer);
  }

// STEFAN: performance hack, not using the lock here is usually safe, depending on the queue implementation
/* On OSX that it is safe, the queue is based on a deque which uses a 
   pointer compare, the object storing the pointers is allocated
   on initalization,
   so there is a race, but there is no problem with corrupted memory.
   the result might be wrong (ignoring avilable data), but it will not crash,
   and it is a lot faster */
#define _SKIP_HAS_DATA_LOCKING 1
  
#if _SKIP_HAS_DATA_LOCKING
  __attribute__((noinline)) // seems to be necessary, otherwise the volatile hack does not work
#endif
  bool hasData() {
    if (not _SKIP_HAS_DATA_LOCKING) OS_Interface::mutex_lock(&lock);
    
    // assign to volatile bool to avoid to optimize that out of any loop
    // does on the tested GCC 4.5 only work if the function is not inlined
    volatile bool result = !channel.empty();  

    if (not _SKIP_HAS_DATA_LOCKING) OS_Interface::mutex_unlock(&lock);

    return result;
  }

};

