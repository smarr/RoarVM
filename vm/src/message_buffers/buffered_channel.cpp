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


#include <string.h>
#include <assert.h> //make sure it is only included if we are using the unittests

#include "buffered_channel.h"

void BufferedChannel::initialize(size_t numberOfBuffers, size_t sizeOfSingleBuffer) {
  syncedqueue_initialize(&free_list, (int32_t*)free_list_buffer, numberOfBuffers);
  syncedqueue_initialize(&waiting_list, (int32_t*)waiting_list_buffer, numberOfBuffers);
  syncedqueue_initialize(&used_list, (int32_t*)used_list_buffer, numberOfBuffers);

  for (size_t i = 0; i < numberOfBuffers; i++) {
    int32_t buffer_address = int32_t(buffer_memory) + i * (sizeOfSingleBuffer + sizeof(buffer));
    syncedqueue_enqueue(&free_list, &buffer_address, 1);
  }
}

void BufferedChannel::send(const void* data, size_t size) {
  // check size of data
  assert(size <= size_of_single_buffer);

  // aquire buffer
  buffer* buffer;
  syncedqueue_dequeue(&free_list, (int32_t*)&buffer, 1);

  // copy data to buffer
  memcpy(buffer->buffer, data, size);
  buffer->used = size;

  // put buffer into used list
  syncedqueue_enqueue(&waiting_list, (int32_t*)&buffer, 1);
}

const void* BufferedChannel::receive(size_t& size) {
  buffer* buffer;

  // block until data available
  syncedqueue_dequeue(&waiting_list, (int32_t*)&buffer, 1);

  // put into used list
  syncedqueue_enqueue(&used_list, (int32_t*)&buffer, 1);

  size = buffer->used;
  return (const void*)buffer->buffer;
}

void BufferedChannel::releaseOldest(void* buffer_to_be_released_for_debugging) {
  // get latest used buffer
  buffer* buffer;

  syncedqueue_dequeue(&used_list, (int32_t*)&buffer, 1);
  assert((void*)buffer->buffer == buffer_to_be_released_for_debugging);

  // set used to 0
  buffer->used = 0;

  // enqueue to free list
  syncedqueue_enqueue(&free_list, (int32_t*)&buffer, 1);
}

bool BufferedChannel::hasData() {
  // check whether waiting list is not empty
  return !syncedqueue_is_empty(&waiting_list);
}

