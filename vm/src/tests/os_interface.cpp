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

# include "headers.h"


/**
 * Test the semantics of atomic_fetch_and_add
 */
TEST(OS_Interface, AtomicFetchAndAdd) {
  int i = 42;
  
  
  ASSERT_EQ(42,     OS_Interface::atomic_fetch_and_add(&i, 4));
  ASSERT_EQ(42 + 4, i);
  
  ASSERT_EQ(42 + 4, OS_Interface::atomic_fetch_and_add(&i, 5));
  
  ASSERT_EQ(42 + 4 + 5, i);
}


/**
 * Test the semantics of atomic_compare_and_swap
 */
TEST(OS_Interface, AtomicCompareAndSwap) {
  int i = 42;
  
  ASSERT_FALSE(OS_Interface::atomic_compare_and_swap(&i, 33, 66));
  ASSERT_EQ(42, i);  // Remains unchanged on failure
  
  ASSERT_TRUE(OS_Interface::atomic_compare_and_swap(&i, 42, 66));
  ASSERT_EQ(66, i);  // Only changed when old_value == i
}


/**
 * Test the semantics of atomic_compare_and_swap_val
 */
TEST(OS_Interface, AtomicCompareAndSwapVal) {
  int i = 42;
  
  ASSERT_EQ(42, OS_Interface::atomic_compare_and_swap_val(&i, 33, 66));
  ASSERT_EQ(42, i);  // Remains unchanged on failure
  
  ASSERT_EQ(42, OS_Interface::atomic_compare_and_swap_val(&i, 42, 66));
  ASSERT_EQ(66, i);  // Only changed when old_value == i
}

/**
 * Test the semantics of least_significant_one
 */
TEST(OS_Interface, LeastSignificantOne) {
  uint64_t zero = 0LL;
  ASSERT_EQ(0, OS_Interface::least_significant_one(zero));
  
  uint64_t three = 1LL << 2;
  ASSERT_EQ(3, OS_Interface::least_significant_one(three));
  
  uint64_t thirtythree = 1LL << 32;
  ASSERT_EQ(33, OS_Interface::least_significant_one(thirtythree));
  
}
