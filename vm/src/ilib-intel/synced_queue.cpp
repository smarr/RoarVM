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


#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <pthread.h>
#include <algorithm>

#include <assert.h>

#include "synced_queue.h"

# ifndef __APPLE__
  # define pthread_yield_np pthread_yield
# endif

void syncedqueue_initialize(p_syncedqueue sq, int32_t* const buffer, size_t item_count) {
  assert(item_count <= USHRT_MAX);
  assert(sizeof(atomic_status) == sizeof(int32_t));

  sq->buffer      = buffer;
  sq->buffer_end  = sq->buffer + item_count;

	sq->writer.wrt.write_offset = 0;
  sq->writer.wrt.free_items   = item_count;

  sq->reader.rd.read_offset = 0;
  sq->reader.rd.avail_items = 0;

  sq->max_number_of_items = item_count;

  sq->initialized = true;
}

/**
 * Helper function to do the acutal copying into the ringbuffer
 */
void _store(p_syncedqueue sq, uint16_t start_offset, int32_t* const data, const size_t item_count) {
  // do we need a wrap around?
  if (start_offset + item_count > sq->max_number_of_items) {
    uint16_t num_items_tail = sq->max_number_of_items - start_offset;   // this number of items is going to the tail
    uint16_t num_items_head = item_count - num_items_tail;              // this number of items is going to the head

    int32_t* writer = sq->buffer + start_offset;
    int32_t* reader = data;

    while (num_items_tail--) {
      *writer++ = *reader++;
    }

    // wrap around and at the front of the ring buffer
    writer = sq->buffer;
    while (num_items_head--) {
      *writer++ = *reader++;
    }
  }
  else {
    uint16_t num_items = item_count;

    int32_t* writer = sq->buffer + start_offset;
    int32_t* reader = data;

    while (num_items--) {
      *writer++ = *reader++;
    }
  }
}

/**
 * Helper function to do the acutal copying out of ringbuffer
 *
 * !! COPY_PAST + swap reader/writer
 *  copy is otherwise identical to _store
 */
void _read(p_syncedqueue sq, const uint16_t start_offset, int32_t* const data, const size_t item_count) {
  // do we need a wrap around?
  if (start_offset + item_count > sq->max_number_of_items) {
    uint16_t num_items_tail = sq->max_number_of_items - start_offset;   // this number of items is going to the tail
    uint16_t num_items_head = item_count - num_items_tail;              // this number of items is going to the head

    int32_t* reader = sq->buffer + start_offset;
    int32_t* writer = data;

    while (num_items_tail--) {
      *writer++ = *reader++;
    }

    // wrap around and at the front of the ring buffer
    reader = sq->buffer;
    while (num_items_head--) {
      *writer++ = *reader++;
    }
  }
  else {
    uint16_t num_items = item_count;

    int32_t* reader = sq->buffer + start_offset;
    int32_t* writer = data;

    while (num_items--) {
      *writer++ = *reader++;
    }
  }
}


/**
 We dont allow requests larger then the buffer size anymore.
 For all relevant usages in the RVM i.e. messages, we can effort
 to have a buffer of a size larger then the largest message
 */
/* void syncedqueue_enqueue_ext(p_syncedqueue sq, int32_t* const data, const size_t item_count) {
 ...
}*/


void syncedqueue_enqueue(p_syncedqueue sq, int32_t* const data, const size_t item_count) {
  assert(item_count <= sq->max_number_of_items);

  bool done = false;

  uint32_t numTries = 0;

  while (!done) {
    atomic_status cur_writer = sq->writer;

    if (cur_writer.wrt.free_items >= item_count) {
      atomic_status new_writer;

      new_writer.wrt.write_offset =
          (cur_writer.wrt.write_offset + item_count) % sq->max_number_of_items;
      new_writer.wrt.free_items = cur_writer.wrt.free_items - item_count;

      if (__sync_bool_compare_and_swap(&sq->writer.atomic_value,
                                       cur_writer.atomic_value,
                                       new_writer.atomic_value)) {
        _store(sq, cur_writer.wrt.write_offset, data, item_count);
        done = true;

        // update available items
        bool updateAvail = false;

        while (!updateAvail) {
          atomic_status reader = sq->reader;
          atomic_status new_reader = reader;
          new_reader.rd.avail_items += item_count;

          if (__sync_bool_compare_and_swap(&sq->reader.atomic_value,
                                           reader.atomic_value,
                                           new_reader.atomic_value)) {
            updateAvail = true;
          }
          //else {
          //  pthread_yield_np();
          //}
        }
      }
    }
    else {
      // STEFAN: check here with a textbook what the most efficient strategie is to wait
      numTries++;

      //if (numTries > 32) {
      //  useconds_t sleep = 1 << (numTries - 32); // wait an expentially growing time span
      //  usleep(std::min((useconds_t)500000, sleep));              // do not sleep longer than 0.5sec
      //}
      //else if (numTries > 16) {
      //  pthread_yield_np();
      //}
      //else { /* NOP */ }
    }
  }
}



/**
 * Dequeuing elements safely with multiple readers
 */
void syncedqueue_dequeue(p_syncedqueue sq, int32_t* const data, const size_t item_count) {
  assert(item_count <= sq->max_number_of_items);

  bool done = false;

  uint32_t numTries = 0;

  while (!done) {
    atomic_status reader = sq->reader;

    if (reader.rd.avail_items >= item_count) {
      atomic_status new_reader;

      new_reader.rd.read_offset = (reader.rd.read_offset + item_count) % sq->max_number_of_items;
      new_reader.rd.avail_items =  reader.rd.avail_items - item_count;

      if (__sync_bool_compare_and_swap(&sq->reader.atomic_value,
                                       reader.atomic_value,
                                       new_reader.atomic_value)) {
        _read(sq, reader.rd.read_offset, data, item_count);
        done = true;

        // update free items
        bool updateFree = false;

        while (!updateFree) {
          atomic_status writer = sq->writer;
          atomic_status new_writer = writer;
          new_writer.wrt.free_items += item_count;

          if (__sync_bool_compare_and_swap(&sq->writer.atomic_value,
                                           writer.atomic_value,
                                           new_writer.atomic_value)) {
            updateFree = true;
          }
          //else {
          //  pthread_yield_np();
          //}
        }
      }
    }
    else {
      // STEFAN: check here with a textbook what the most efficient strategie is to wait
      numTries++;

      if (numTries > 32) {
        useconds_t sleep = 1 << (numTries - 32); // wait an expentially growing time span
        usleep(std::min((useconds_t)500000, sleep));              // do not sleep longer than 0.5sec
      }
      else if (numTries > 16) {
        pthread_yield_np();
      }
      else { /* NOP */ }
    }
  }
}


bool syncedqueue_is_initialized(p_syncedqueue sq) {
  return sq->initialized;
}

bool syncedqueue_is_empty(p_syncedqueue sq) {
  return sq->reader.rd.avail_items == 0;
}

bool syncedqueue_is_full(p_syncedqueue sq) {
  return sq->reader.wrt.free_items == 0;
}

