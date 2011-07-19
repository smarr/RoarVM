/******************************************************************************
 *  Copyright (c) 2008 - 2011 IBM Corporation and others.
 *  All rights reserved. This program and the accompanying materials
 *  are made available under the terms of the Eclipse Public License v1.0
 *  which accompanies this distribution, and is available at
 *  http://www.eclipse.org/legal/epl-v10.html
 * 
 *  Contributors:
 *    Stefan Marr, Vrije Universiteit Brussel - Initial Implementation
 ******************************************************************************/


#include <gtest/gtest.h>

# include "headers.h"


TEST(Interprocess_Allocator, PadForWordAlignment) {
  ASSERT_EQ(0,  Interprocess_Allocator::pad_for_word_alignment(0));
  ASSERT_EQ(4,  Interprocess_Allocator::pad_for_word_alignment(1));
  ASSERT_EQ(4,  Interprocess_Allocator::pad_for_word_alignment(2));
  ASSERT_EQ(4,  Interprocess_Allocator::pad_for_word_alignment(3));
  ASSERT_EQ(4,  Interprocess_Allocator::pad_for_word_alignment(4));
  
  ASSERT_EQ(8,  Interprocess_Allocator::pad_for_word_alignment(5));
  ASSERT_EQ(8,  Interprocess_Allocator::pad_for_word_alignment(6));
  ASSERT_EQ(8,  Interprocess_Allocator::pad_for_word_alignment(7));
  ASSERT_EQ(8,  Interprocess_Allocator::pad_for_word_alignment(8));

  ASSERT_EQ(12,  Interprocess_Allocator::pad_for_word_alignment(9));
  
  ASSERT_EQ(128,  Interprocess_Allocator::pad_for_word_alignment(127));
  
  ASSERT_EQ(256,  Interprocess_Allocator::pad_for_word_alignment(255));
  ASSERT_EQ(256,  Interprocess_Allocator::pad_for_word_alignment(256));
  ASSERT_EQ(260,  Interprocess_Allocator::pad_for_word_alignment(257));
}


TEST(Interprocess_Allocator, BasicAllocation) {
  void* mem = malloc(512);
  
  Interprocess_Allocator ia(mem, 512);
  
  for (size_t i = 0; i < 64; i++) {
    ASSERT_NE(ia.allocate(4), (void*)NULL);
  }
  
  ASSERT_EQ(NULL, ia.allocate(4));
  
  free(mem);
}

TEST(Interprocess_Allocator, AllocationAndDeallocation) {
  void* mem = malloc(512);
  
  Interprocess_Allocator ia(mem, 512);
  
  void* m1 = ia.allocate(4);
  void* m2 = ia.allocate(4);
  void* m3 = ia.allocate(4);
  void* m4 = ia.allocate(4);
  
  for (size_t i = 0; i < 60; i++) {
    ASSERT_NE((void*)NULL, ia.allocate(4));
  }
  
  ASSERT_EQ((void*)NULL, ia.allocate(4));  // too much requested
  
  ia.free(m1);
  ASSERT_EQ((void*)NULL, ia.allocate(8));  // too much requested
  ASSERT_NE((void*)NULL, ia.allocate(4));

  ia.free(m2);
  ia.free(m3);
  ASSERT_EQ((void*)NULL, ia.allocate(16));  // too much requested, so should fail
  ASSERT_NE((void*)NULL, ia.allocate(12));  // that should be ok
  ASSERT_EQ((void*)NULL, ia.allocate(4));   // too much requested
  
  ia.free(m4);
  ASSERT_EQ((void*)NULL, ia.allocate(8));   // too much requested
  ASSERT_NE((void*)NULL, ia.allocate(4));

  ASSERT_EQ((void*)NULL, ia.allocate(4));   // all full again -> failing allocate
  free(mem);
}


TEST(Interprocess_Allocator, NonOverlapping) {
  void* mem = malloc(512);
  int* allocated_elements[64];
  
  Interprocess_Allocator ia(mem, 512);
  
  for (size_t i = 0; i < 64; i++) {
    allocated_elements[i] = (int*)ia.allocate(sizeof(int));
    ASSERT_NE((void*)NULL, allocated_elements[i]);
    *allocated_elements[i] = i;
  }
  
  for (size_t i = 0; i < 64; i++) {
    ASSERT_NE(i, *allocated_elements[i]);
  }
  
  free(mem);
}


