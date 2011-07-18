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
 *  Implemented with a fixed size ring buffer.
 *  The buffer containts elements of a given size (sizeof(int32_t) for the moment).
 *  It can contain max. 64k elements.
 *  This is restricted by the available 32-bit compare-and-swap operation.
 *  The lock-free implementation is based on strong the writing offset and
 *  the number of free items in an int32_t to allow atomic updates on CPU level.
 *
 *  Origninally inspired by Ring_Buffer, 1997-06-19, Jarno Elonen <elonen@iki.fi>
 *  http://elonen.iki.fi/code/misc-notes/ringbuffer/
 *  Current implementation is completely rewritten to be entierly lock-free and
 *  supports multi-writers and a single-reader.
 *
 ******************************************************************************/


#include <pthread.h>
#include <stdint.h>

#ifndef __SYNCED_QUEUE_H__
#define __SYNCED_QUEUE_H__

typedef union {

  int32_t atomic_value;

  struct {
    uint16_t write_offset;  // offset for the write pointer into the ringbuffer
    uint16_t free_items;    // number of free entries in the buffer, an entry should be a int32_t i.e. 4byte
  } wrt;

  struct {
    uint16_t read_offset;   // offset for the read pointer into the ringbuffer
    uint16_t avail_items;   // number of available entries in the buffer, an entry should be a int32_t i.e. 4byte
  } rd;

} atomic_status;


typedef struct syncedqueue {
  int32_t* buffer;        // pointer of the start of the buffer (read-only)
  int32_t* buffer_end;    // pointer to the end of the buffer   (read-only)

  atomic_status writer;   // status data for all writers
  atomic_status reader;   // status data for all readers

	uint16_t max_number_of_items;

  bool initialized;
} syncedqueue, *p_syncedqueue;

/**
 * Initialize the given queue.
 *    buffer     - pointer to the buffer of int32_t elements
 *    item_count - max number of items fitting into the buffer
 */
void syncedqueue_initialize(p_syncedqueue sq,   int32_t* const buffer, size_t item_count);

void syncedqueue_enqueue(p_syncedqueue sq,      int32_t* const data, const size_t item_count);
void syncedqueue_dequeue(p_syncedqueue sq,      int32_t* const data, const size_t item_count);
bool syncedqueue_is_initialized(p_syncedqueue sq);
bool syncedqueue_is_empty(p_syncedqueue sq);

#endif

