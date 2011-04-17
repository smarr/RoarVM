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

#include "synced_queue.h"

#include <gtest/gtest.h>

void _initBufferWithNumberSequence(int32_t* buffer, size_t size) {
  for (size_t i = 0; i < size; i++) {
    buffer[i] = i;
  }
}

TEST(SyncedQueue, Init) {
  syncedqueue sq = { 0 }; // init with 0 to allow proper check of initialization
  int32_t buffer[10];

  EXPECT_FALSE(syncedqueue_is_initialized(&sq));

  syncedqueue_initialize(&sq, buffer, 10);

  EXPECT_TRUE(syncedqueue_is_initialized(&sq));
}

TEST(SyncedQueue, SimpleEnqueueDequeue) {
  syncedqueue sq = { 0 }; // init with 0 to allow proper check of initialization
  int32_t buffer[10];
  syncedqueue_initialize(&sq, buffer, 10);
  _initBufferWithNumberSequence(buffer, 10);

  int32_t sample[4] = { 42, 21, 11, 4 };

  syncedqueue_enqueue(&sq, sample, 4);

  // implementation specific, but I like to check that it works already here
  for (size_t i = 0; i < 4; i++) {
    EXPECT_EQ(sample[i], buffer[i]);
  }

  int32_t dequeuedSample[4] = { 0 };

  syncedqueue_dequeue(&sq, dequeuedSample, 4);

  for (size_t i = 0; i < 4; i++) {
    EXPECT_EQ(sample[i], dequeuedSample[i]);
  }
}

TEST(SyncedQueue, IsEmpty) {
  syncedqueue sq = { 0 }; // init with 0 to allow proper check of initialization
  int32_t buffer[10] = { 0 };

  syncedqueue_initialize(&sq, buffer, 10);
  EXPECT_TRUE(syncedqueue_is_empty(&sq));

  int32_t sample[4] = { 0 };
  syncedqueue_enqueue(&sq, sample, 4);
  EXPECT_FALSE(syncedqueue_is_empty(&sq));

  int32_t dequeuedSample[4] = { 0 };
  syncedqueue_dequeue(&sq, dequeuedSample, 4);
  EXPECT_TRUE(syncedqueue_is_empty(&sq));
}

TEST(SyncedQueue, SubsequentEnqueueDequeue) {
  syncedqueue sq = { 0 }; // init with 0 to allow proper check of initialization
  int32_t buffer[10];
  syncedqueue_initialize(&sq, buffer, 10);
  _initBufferWithNumberSequence(buffer, 10);

  int32_t sample[5] = { 42, 21, 11, 4, 754 };
  syncedqueue_enqueue(&sq, sample, 5);
  // implementation specific, but I like to check that it works already here
  for (size_t i = 0; i < 5; i++) {
    EXPECT_EQ(sample[i], buffer[i]);
  }

  int32_t sample2[5] = { 55, 66, 77, 88, 99 };
  syncedqueue_enqueue(&sq, sample2, 5);
  for (size_t i = 0; i < 5; i++) {
    EXPECT_EQ(sample2[i], buffer[i + 5]);
  }

  int32_t dequeuedSample[5] = { 0 };
  syncedqueue_dequeue(&sq, dequeuedSample, 5);
  for (size_t i = 0; i < 5; i++) {
    EXPECT_EQ(sample[i], dequeuedSample[i]);
  }

  int32_t sample3[5] = { 7, 8, 9, 0, 1 };
  syncedqueue_enqueue(&sq, sample3, 5);
  for (size_t i = 0; i < 5; i++) {
    EXPECT_EQ(sample3[i], buffer[i]);
  }

  // second time
  syncedqueue_dequeue(&sq, dequeuedSample, 5);
  for (size_t i = 0; i < 5; i++) {
    EXPECT_EQ(sample2[i], dequeuedSample[i]);
  }

  // 3rd time
  syncedqueue_dequeue(&sq, dequeuedSample, 5);
  for (size_t i = 0; i < 5; i++) {
    EXPECT_EQ(sample3[i], dequeuedSample[i]);
  }

}

TEST(SyncedQueue, WrappingEnqueueDequeue) {
  syncedqueue sq = { 0 }; // init with 0 to allow proper check of initialization
  int32_t buffer[10];
  syncedqueue_initialize(&sq, buffer, 10);
  _initBufferWithNumberSequence(buffer, 10);

  int32_t sample[7] = { 42, 21, 11, 4, 754, 33, 21 };
  syncedqueue_enqueue(&sq, sample, 7);
  for (size_t i = 0; i < 7; i++) {
    EXPECT_EQ(sample[i], buffer[i]);
  }

  int32_t dequeuedSample[10] = { 0 };
  syncedqueue_dequeue(&sq, dequeuedSample, 7);
  for (size_t i = 0; i < 7; i++) {
    EXPECT_EQ(sample[i], dequeuedSample[i]);
  }

  int32_t sample2[5] = { 77, 88, 99, -1, 11 };
  syncedqueue_enqueue(&sq, sample2, 5);
  EXPECT_EQ(sample2[0], buffer[7]);
  EXPECT_EQ(sample2[1], buffer[8]);
  EXPECT_EQ(sample2[2], buffer[9]);
  EXPECT_EQ(sample2[3], buffer[0]);
  EXPECT_EQ(sample2[4], buffer[1]);

  syncedqueue_dequeue(&sq, dequeuedSample, 5);
  for (size_t i = 0; i < 5; i++) {
    EXPECT_EQ(sample2[i], dequeuedSample[i]);
  }
}

# endif // !On_Tilera

