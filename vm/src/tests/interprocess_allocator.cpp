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
