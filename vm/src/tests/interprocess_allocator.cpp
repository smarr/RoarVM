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


TEST(Interprocess_Allocator, FreeItem) {
  Interprocess_Allocator::Item free_item;
  memset(&free_item, 0, sizeof(free_item));
  
  ASSERT_FALSE(free_item.is_actually_free_item());
  
  free_item.set_size(4);
  ASSERT_FALSE(free_item.is_actually_free_item());
  
  free_item.become_free();
  ASSERT_TRUE (free_item.is_actually_free_item());
  ASSERT_EQ(4, free_item.get_size());
  
  free_item.become_used();
  ASSERT_EQ(4, free_item.get_size());
  ASSERT_FALSE(free_item.is_actually_free_item());
}

TEST(Interprocess_Allocator, UsedItemContent) {
  Interprocess_Allocator::Item item;
  
  // The round-trip should work as expected
  
  void* content = item.get_content();
  ASSERT_NE(&item, content);
  
  ASSERT_EQ(&item, Interprocess_Allocator::Item::from_content(content));
}


TEST(Interprocess_Allocator, PadForWordAlignment) {
  // We need to guarantee some additionally free space for the management of the free items
  // since this is done inplace.
  ASSERT_EQ(12,  Interprocess_Allocator::Item::manage_and_pad(0));
  ASSERT_EQ(12,  Interprocess_Allocator::Item::manage_and_pad(3));
  
  ASSERT_EQ(12,  Interprocess_Allocator::Item::manage_and_pad(7));
  ASSERT_EQ(12,  Interprocess_Allocator::Item::manage_and_pad(8));
  ASSERT_EQ(16,  Interprocess_Allocator::Item::manage_and_pad(9));
  
  ASSERT_EQ(16,  Interprocess_Allocator::Item::manage_and_pad(12));
  ASSERT_EQ(20,  Interprocess_Allocator::Item::manage_and_pad(13));
  
  ASSERT_EQ(20,  Interprocess_Allocator::Item::manage_and_pad(16));
  ASSERT_EQ(24,  Interprocess_Allocator::Item::manage_and_pad(17));
  
  ASSERT_EQ(256, Interprocess_Allocator::Item::manage_and_pad(251));
  ASSERT_EQ(256, Interprocess_Allocator::Item::manage_and_pad(252));
  ASSERT_EQ(260, Interprocess_Allocator::Item::manage_and_pad(253));
}


TEST(Interprocess_Allocator, BasicAllocation) {
  void* mem = malloc(512);
  
  Interprocess_Allocator ia(mem, 512);
  
  for (size_t i = 0; i < 40; i++) {        // not 64 elements in total because we do not split very small elements
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
  for (size_t i = 0; i < 41; i++) {     // not 64 elements in total because we do not split very small elements
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
  
  for (size_t i = 0; i < 37; i++) {        // not 64 elements in total because we do not split very small elements
    ASSERT_NE((void*)NULL, ia.allocate(4));
  }
  
  ASSERT_EQ((void*)NULL, ia.allocate(4));  // too much requested
  
  ia.free(m1);
  ASSERT_EQ((void*)NULL, ia.allocate(42)); // there should be 32bytes left, so not enough
  ASSERT_NE((void*)NULL, ia.allocate(4));

  ia.free(m2);
  ia.free(m3);
  ASSERT_EQ((void*)NULL, ia.allocate(16)); // too much segmentation
  ASSERT_NE((void*)NULL, ia.allocate(4));  // that should be ok
  ASSERT_NE((void*)NULL, ia.allocate(4));  // too much requested
  
  ia.free(m4);
  ASSERT_EQ((void*)NULL, ia.allocate(32)); // too much requested
  ASSERT_NE((void*)NULL, ia.allocate(4));

  ASSERT_EQ((void*)NULL, ia.allocate(4));  // all full again -> failing allocate
  free(mem);
}


TEST(Interprocess_Allocator, NonOverlapping) {
  void* mem = malloc(512);
  int* allocated_elements[41];
  
  Interprocess_Allocator ia(mem, 512);
  
  for (size_t i = 0; i < 41; i++) {         // not 64 elements in total because we do not split very small elements
    allocated_elements[i] = (int*)ia.allocate(sizeof(int));
    ASSERT_NE((void*)NULL, allocated_elements[i]);
    *allocated_elements[i] = i;
  }
  
  for (size_t i = 0; i < 41; i++) {
    ASSERT_EQ(i, *allocated_elements[i]);
  }
  
  free(mem);
}

