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
 ******************************************************************************/


#include <stdint.h>
#include <stdlib.h>

#include "synced_queue.h"

class BufferedChannel {
private:
  const void* buffer_memory;
  const size_t num_buffers;
  const size_t size_of_single_buffer;

  typedef struct buffer {
    size_t used;
    char buffer[0];
  } buffer, *pBuffer;

  syncedqueue free_list;          // free buffers to be used
  const void* free_list_buffer;

  syncedqueue waiting_list;       // waiting buffers to be received, have been free before, will be come used after receiving
  const void* waiting_list_buffer;

  syncedqueue used_list;          // buffers used in the client program, can be released to free buffers
  const void* used_list_buffer;

  void initialize(size_t numberOfBuffers, size_t sizeOfSingleBuffer);

public:
  BufferedChannel(size_t numberOfBuffers, size_t sizeOfSingleBuffer)
  : buffer_memory(malloc(numberOfBuffers * (sizeOfSingleBuffer + sizeof(buffer)))),
    num_buffers(numberOfBuffers),
	size_of_single_buffer(sizeOfSingleBuffer),
	free_list_buffer(malloc(sizeof(int32_t) * numberOfBuffers)),
    waiting_list_buffer(malloc(sizeof(int32_t) * numberOfBuffers)),
    used_list_buffer(malloc(sizeof(int32_t) * numberOfBuffers))
  {
    //num_buffers = numberOfBuffers;
    initialize(numberOfBuffers, sizeOfSingleBuffer);
  }

  ~BufferedChannel() {
    free((void*)buffer_memory);
    free((void*)free_list_buffer);
    free((void*)waiting_list_buffer);
    free((void*)used_list_buffer);
  }

  void send(const void* data, size_t size);
  const void* receive(size_t& size);
  void releaseOldest(void*);
  bool hasData();

};

