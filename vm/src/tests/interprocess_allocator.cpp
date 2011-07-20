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


# include <gtest/gtest.h>

# ifdef Debugging
# undef Debugging
# endif

# define Debugging 1

# include "headers.h"


TEST(Interprocess_Allocator, FreeItemAndUsedItem) {
  Interprocess_Allocator::Free_Item free_item;
  memset(&free_item, 0, sizeof(free_item));
  
  ASSERT_FALSE(free_item.is_actually_free_item());
  
  free_item.set_size(4);
  ASSERT_TRUE (free_item.is_actually_free_item());
  ASSERT_EQ(4, free_item.get_size());
  
  Interprocess_Allocator::Used_Item* used_item = (Interprocess_Allocator::Used_Item*)&free_item;
  
  ASSERT_TRUE (used_item->is_actually_free_item());
  ASSERT_EQ(4, used_item->get_size());
  
  used_item->set_size(8);
  ASSERT_EQ(8, used_item->get_size());
  ASSERT_FALSE(used_item->is_actually_free_item());
  
  ASSERT_FALSE(free_item.is_actually_free_item());
  ASSERT_EQ(8, free_item.get_size());
}

TEST(Interprocess_Allocator, UsedItemContent) {
  Interprocess_Allocator::Used_Item item;
  
  // The round-trip should work as expected
  
  void* content = item.get_content();
  ASSERT_NE(&item, content);
  
  ASSERT_EQ(&item, Interprocess_Allocator::Used_Item::from_content(content));
}


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
  
  for (size_t i = 0; i < 60; i++) {        // not 64 elements in total because we do not split very small elements
    ASSERT_NE(ia.allocate(4), (void*)NULL);
  }
  ASSERT_NE(ia.allocate(4), (void*)NULL);  // this is the last one because of the hyristic that is ment to reduce fragmentation
  
  ASSERT_EQ(NULL, ia.allocate(4));
  
  free(mem);
}

TEST(Interprocess_Allocator, UniqueAllocationResultsInMemRegion) {
  void* mem = malloc(512);
  Interprocess_Allocator ia(mem, 512);
  
  void* prev = NULL;
  for (size_t i = 0; i < 61; i++) {     // not 64 elements in total because we do not split very small elements
    void* result = ia.allocate(4);
    
    ASSERT_NE(result, (void*)NULL);
    ASSERT_NE(result, prev);
    
    ASSERT_TRUE(result >= mem);
    ASSERT_TRUE(((intptr_t)result + 4) <= ((intptr_t)mem + 512));
    
    prev = result;
  }
  
  free(mem);
}

TEST(Interprocess_Allocator, AllocationAndDeallocation) {
  void* mem = malloc(512);
  
  Interprocess_Allocator ia(mem, 512);
  
  void* m1 = ia.allocate(4);
  void* m2 = ia.allocate(4);
  void* m3 = ia.allocate(4);
  void* m4 = ia.allocate(4);
  
  for (size_t i = 0; i < 57; i++) {       // not 64 elements in total because we do not split very small elements
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
  int* allocated_elements[61];
  
  Interprocess_Allocator ia(mem, 512);
  
  for (size_t i = 0; i < 61; i++) {         // not 64 elements in total because we do not split very small elements
    allocated_elements[i] = (int*)ia.allocate(sizeof(int));
    ASSERT_NE((void*)NULL, allocated_elements[i]);
    *allocated_elements[i] = i;
  }
  
  for (size_t i = 0; i < 61; i++) {
    ASSERT_EQ(i, *allocated_elements[i]);
  }
  
  free(mem);
}

