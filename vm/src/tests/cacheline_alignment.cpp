/******************************************************************************
 *  Copyright (c) 2008 - 2010 IBM Corporation and others.
 *  All rights reserved. This program and the accompanying materials
 *  are made available under the terms of the Eclipse Public License v1.0
 *  which accompanies this distribution, and is available at
 *  http://www.eclipse.org/legal/epl-v10.html
 * 
 *  Contributors:
 *    David Ungar, IBM Research - Initial Implementation
 *    Sam Adams, IBM Research - Initial Implementation
 *    Stefan Marr, Vrije Universiteit Brussel - Port to x86 Multi-Core Systems
 ******************************************************************************/


# include <stdlib.h>
# include <gtest/gtest.h>

# include "cacheline_aligned.h"

TEST(CacheAligned, OneLine) {
  ASSERT_EQ(64, CACHELINE_SIZE);
  ASSERT_EQ(64, sizeof(cacheline_aligned<int>));
  
  ASSERT_EQ(64, sizeof(cacheline_aligned<bool>));
  
  ASSERT_EQ(64, sizeof(cacheline_aligned<char[4]>));
}

TEST(CacheAligned, TwoLines) {
  ASSERT_EQ(64, CACHELINE_SIZE);
  
  ASSERT_EQ(72 , sizeof(int64_t[9]));
  ASSERT_EQ(64 * 2, sizeof(cacheline_aligned<int64_t[9]>));
  
  ASSERT_EQ(64 * 2, sizeof(cacheline_aligned<char[65]>));
}


TEST(CacheAligned, ThreeLines) {
  ASSERT_EQ(64, CACHELINE_SIZE);
  ASSERT_EQ(64 * 3, sizeof(cacheline_aligned<int64_t[17]>));
  
  ASSERT_EQ(64 * 3, sizeof(cacheline_aligned<char[129]>));
  ASSERT_EQ(64 * 3, sizeof(cacheline_aligned<char[191]>));
}

TEST(CacheAligned, Arrays) {
  cacheline_aligned<int> myIntArray[1];
  ASSERT_EQ(64, sizeof(myIntArray));
  
  ASSERT_EQ(64 * 4, sizeof(cacheline_aligned<char>[4]));
  ASSERT_EQ(64 * 15, sizeof(cacheline_aligned<char>[15]));
}

