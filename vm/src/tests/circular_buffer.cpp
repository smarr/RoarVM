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


# include <gtest/gtest.h>

# include "circular_buffer.h"

TEST(Circular_Buffer, EmptyOnCreation) {
  Circular_Buffer<void*> cb(NULL, 1);
  
  ASSERT_TRUE(cb.is_empty());
}

TEST(Circular_Buffer, SimpleEnqueue) {
  void* mem[1];
  
  Circular_Buffer<void*> cb(&mem, 1);
  
  cb.enqueue(NULL);
  ASSERT_FALSE(cb.is_empty());
}

TEST(Circular_Buffer, SimpleEnqueueDequeue) {
  void* mem[2];
  
  Circular_Buffer<void*> cb(&mem, 2);
  
  cb.enqueue(NULL);
  cb.dequeue();
  
  ASSERT_TRUE(cb.is_empty());
}

TEST(Circular_Buffer, AssertOnOverAllocation) {
  void* mem[10];
  
  Circular_Buffer<void*> cb(&mem, 10);
  
  for (size_t i = 0; i < 10; i++)
    cb.enqueue(NULL);
  
  ASSERT_DEATH(cb.enqueue(NULL), "");
}

TEST(Circular_Buffer, NullOnEmptyDequeue) {
  void* mem[1];
  
  Circular_Buffer<void*> cb(&mem, 11);
  
  cb.enqueue((void*)1);
  
  ASSERT_EQ((void*)1, cb.dequeue());
  ASSERT_EQ(NULL, cb.dequeue());
}

TEST(Circular_Buffer, EndlessUsability) {
  void* mem[2];
  
  Circular_Buffer<void*> cb(&mem, 2);
  
  void* a = (void*)1;
  void* b = (void*)2;
  
  cb.enqueue(a);
  cb.enqueue(b);
  
  for (size_t i = 0; i < 100; i++) {
    ASSERT_EQ(a, cb.dequeue());
    ASSERT_FALSE(cb.is_empty());
    a = b;
    b = (void*)i;
    cb.enqueue(b);
  }
  
  ASSERT_EQ(a, cb.dequeue());
  ASSERT_EQ(b, cb.dequeue());
  ASSERT_TRUE(cb.is_empty());
}
