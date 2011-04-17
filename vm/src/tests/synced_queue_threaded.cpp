/******************************************************************************
 *  Copyright (c) 2008 - 2010 IBM Corporation and others.
 *  All rights reserved. This program and the accompanying materials
 *  are made available under the terms of the Eclipse Public License v1.0
 *  which accompanies this distribution, and is available at
 *  http://www.eclipse.org/legal/epl-v10.html
 * 
 *  Contributors:
 *    Stefan Marr, Vrije Universiteit Brussel - Initial Implementation
 ******************************************************************************/


# if !On_Tilera

#include <gtest/gtest.h>
#include <limits.h>

#include "synced_queue.h"
#include "starter.h"

syncedqueue sq = { 0 }; // init with 0 to allow proper check of initialization

const int NUM_PRODUCERS = 2;
const int64_t NUM_ITEMS     = int64_t(USHRT_MAX) * 1024 * 1024;
const int BUF_SIZE      = USHRT_MAX;//NUM_ITEMS * NUM_PRODUCERS;
const int HISTORY       = NUM_PRODUCERS * 1024;

void _procuderThread(thread_param_t* const tp) {
  thread_param_signalAndAwaitInitialization(tp);
  
  for (int64_t i = tp->id * NUM_ITEMS; i < (tp->id + 1) * NUM_ITEMS; i++) {
    int32_t val = (int32_t)i;
    syncedqueue_enqueue(&sq, &val, 1);
  }
}


TEST(SyncedQueueThreaded, DISABLED_Pressure) {
  int32_t buffer[BUF_SIZE];
  syncedqueue_initialize(&sq, buffer, BUF_SIZE);
  EXPECT_TRUE(syncedqueue_is_initialized(&sq));
  
  starter_t starter;
  starter_init(&starter, NUM_PRODUCERS);
  
  pthread_t threads[NUM_PRODUCERS];
  
  starter_spawn_threads(&starter, _procuderThread, (pthread_t*)&threads);
  
  starter_signal_initalization_finished(&starter);
  
  int32_t last_items[HISTORY] = { -1 };
  int64_t remaining_items = NUM_ITEMS * NUM_PRODUCERS;
  int32_t history_current = 0;
  int32_t history_count   = 0;
  
  while (remaining_items > 0) {
    // read the item
    int32_t val;
    syncedqueue_dequeue(&sq, &val, 1);

    // determine whether there are any duplicates in the history
    for (size_t i = 0; i < history_count; i++) {
      EXPECT_NE(last_items[i], val);
      if (last_items[i] == val) {
        printf("Expect failed: hC: %d, i: %lu, remaining: %lld\n", history_count, i, remaining_items);
      }
    }
    
    // place it in the history
    last_items[history_current] = val;
    history_count   = std::min(history_count + 1, HISTORY);
    history_current = (history_current + 1) % HISTORY;

    remaining_items--;
  }
}

# endif // !On_Tilera
