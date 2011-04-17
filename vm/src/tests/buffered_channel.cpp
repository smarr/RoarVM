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
#include "buffered_channel.h"

TEST(BufferedChannelTest, hasData) {
  char sampleData[10] = { 3, 5, 4, 88, 66, 77, 22, 44, 45, 11 };
  BufferedChannel channel(10, 10);

  EXPECT_FALSE(channel.hasData());

  channel.send(sampleData, 10);

  EXPECT_TRUE(channel.hasData());
}

TEST(BufferedChannelTest, releaseOldest) {
  char sampleData[10] = { 3, 5, 4, 88, 66, 77, 22, 44, 45, 11 };
  BufferedChannel channel(10, 10);
  channel.send(sampleData, 10);

  size_t size;
  void* result = (void*)channel.receive(size);

  EXPECT_FALSE(channel.hasData());

  channel.releaseOldest(result);

  EXPECT_FALSE(channel.hasData());
}

TEST(BufferedChannelTest, simpleSend) {
  char sampleData[10] = { 3, 5, 4, 88, 66, 77, 22, 44, 45, 11 };
  const char* receiveBuffer = NULL;

  BufferedChannel channel(10, 10);
  channel.send(sampleData, 10);

  size_t size;
  receiveBuffer = (const char*)channel.receive(size);

  EXPECT_NE((intptr_t)NULL, (intptr_t)receiveBuffer);
  EXPECT_EQ(10, size);

  for (size_t i = 0; i < size; i++) {
    EXPECT_EQ(sampleData[i], receiveBuffer[i]);
  }
}

TEST(BufferedChannelTest, simpleSendReleaseSend) {
  char sampleData[10] = { 3, 5, 4, 88, 66, 77, 22, 44, 45, 11 };
  const char* receiveBuffer = NULL;

  BufferedChannel channel(1, 10);
  channel.send(sampleData, 10);

  size_t size;
  receiveBuffer = (const char*)channel.receive(size);

  EXPECT_NE((intptr_t)NULL, (intptr_t)receiveBuffer);
  EXPECT_EQ(10, size);

  for (size_t i = 0; i < size; i++) {
    EXPECT_EQ(sampleData[i], receiveBuffer[i]);
  }

  channel.releaseOldest((void*)receiveBuffer);

  char sampleData2[10] = { 9, 1, 8, 7, 6, 5, 4, 3, 2, 1 };
  channel.send(sampleData2, 10);

  receiveBuffer = (const char*)channel.receive(size);

  EXPECT_NE((intptr_t)NULL, (intptr_t)receiveBuffer);
  EXPECT_EQ(10, size);

  for (size_t i = 0; i < size; i++) {
    EXPECT_EQ(sampleData2[i], receiveBuffer[i]);
  }

}

# endif // !On_Tilera
