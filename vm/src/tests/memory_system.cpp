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

TEST(Memory_System_Page_Number, power_of_2_group_size) {
  Memory_System::min_heap_MB = 128;
  Logical_Core::group_size   = 1;

  ASSERT_EQ(128 / 16, Memory_System::calculate_total_read_write_pages(LARGE_PAGE_SIZE));
  ASSERT_EQ(128 / 16, Memory_System::calculate_total_read_mostly_pages(LARGE_PAGE_SIZE));
  
  Logical_Core::group_size   = 2;
  ASSERT_EQ(128 / 16, Memory_System::calculate_total_read_write_pages(LARGE_PAGE_SIZE));
  ASSERT_EQ(128 / 16, Memory_System::calculate_total_read_mostly_pages(LARGE_PAGE_SIZE));

  Logical_Core::group_size   = 4;
  ASSERT_EQ(128 / 16, Memory_System::calculate_total_read_write_pages(LARGE_PAGE_SIZE));
  ASSERT_EQ(128 / 16, Memory_System::calculate_total_read_mostly_pages(LARGE_PAGE_SIZE));
  
  Logical_Core::group_size   = 8;
  ASSERT_EQ(128 / 16, Memory_System::calculate_total_read_write_pages(LARGE_PAGE_SIZE));
  ASSERT_EQ(128 / 16, Memory_System::calculate_total_read_mostly_pages(LARGE_PAGE_SIZE));

  Logical_Core::group_size   = 16;
  ASSERT_EQ(16, Memory_System::calculate_total_read_write_pages(LARGE_PAGE_SIZE));
  ASSERT_EQ( 8, Memory_System::calculate_total_read_mostly_pages(LARGE_PAGE_SIZE));
  
  Logical_Core::group_size   = 32;
  ASSERT_EQ(32, Memory_System::calculate_total_read_write_pages(LARGE_PAGE_SIZE));
  ASSERT_EQ( 8, Memory_System::calculate_total_read_mostly_pages(LARGE_PAGE_SIZE));

  Logical_Core::group_size   = 64;
  ASSERT_EQ(64, Memory_System::calculate_total_read_write_pages(LARGE_PAGE_SIZE));
  ASSERT_EQ( 8, Memory_System::calculate_total_read_mostly_pages(LARGE_PAGE_SIZE));
}

TEST(Memory_System_Page_Number, even_group_sizes) {
  Memory_System::min_heap_MB = 128;
  
  Logical_Core::group_size   = 6;
  EXPECT_EQ(12, Memory_System::calculate_total_read_write_pages(LARGE_PAGE_SIZE));
  EXPECT_EQ(12, Memory_System::calculate_total_read_mostly_pages(LARGE_PAGE_SIZE));

  Logical_Core::group_size   = 10;
  EXPECT_EQ(10, Memory_System::calculate_total_read_write_pages(LARGE_PAGE_SIZE));
  EXPECT_EQ(10, Memory_System::calculate_total_read_mostly_pages(LARGE_PAGE_SIZE));

  Logical_Core::group_size   = 12;
  EXPECT_EQ(12, Memory_System::calculate_total_read_write_pages(LARGE_PAGE_SIZE));
  EXPECT_EQ(12, Memory_System::calculate_total_read_mostly_pages(LARGE_PAGE_SIZE));

  Logical_Core::group_size   = 14;
  EXPECT_EQ(14, Memory_System::calculate_total_read_write_pages(LARGE_PAGE_SIZE));
  EXPECT_EQ(14, Memory_System::calculate_total_read_mostly_pages(LARGE_PAGE_SIZE));
  
  Logical_Core::group_size   = 22;
  EXPECT_EQ(22, Memory_System::calculate_total_read_write_pages(LARGE_PAGE_SIZE));
  EXPECT_EQ(11, Memory_System::calculate_total_read_mostly_pages(LARGE_PAGE_SIZE));
  
  Logical_Core::group_size   = 24;
  EXPECT_EQ(24, Memory_System::calculate_total_read_write_pages(LARGE_PAGE_SIZE));
  EXPECT_EQ(12, Memory_System::calculate_total_read_mostly_pages(LARGE_PAGE_SIZE));
  
  Logical_Core::group_size   = 48;
  EXPECT_EQ(48, Memory_System::calculate_total_read_write_pages(LARGE_PAGE_SIZE));
  EXPECT_EQ(12, Memory_System::calculate_total_read_mostly_pages(LARGE_PAGE_SIZE));
}


TEST(Memory_System_Page_Number, odd_group_sizes) {
  Memory_System::min_heap_MB = 128;
  Logical_Core::group_size   = 1;
  
  ASSERT_EQ(128 / 16, Memory_System::calculate_total_read_write_pages(LARGE_PAGE_SIZE));
  ASSERT_EQ(128 / 16, Memory_System::calculate_total_read_mostly_pages(LARGE_PAGE_SIZE));
  
  Logical_Core::group_size   = 2;
  ASSERT_EQ(128 / 16, Memory_System::calculate_total_read_write_pages(LARGE_PAGE_SIZE));
  ASSERT_EQ(128 / 16, Memory_System::calculate_total_read_mostly_pages(LARGE_PAGE_SIZE));
  
  Logical_Core::group_size   = 3;
  ASSERT_EQ(12, Memory_System::calculate_total_read_write_pages(LARGE_PAGE_SIZE));
  ASSERT_EQ(12, Memory_System::calculate_total_read_mostly_pages(LARGE_PAGE_SIZE));
  
  Logical_Core::group_size   = 5;
  EXPECT_EQ(10, Memory_System::calculate_total_read_write_pages(LARGE_PAGE_SIZE));
  EXPECT_EQ(10, Memory_System::calculate_total_read_mostly_pages(LARGE_PAGE_SIZE));
  
  Logical_Core::group_size   = 11;
  EXPECT_EQ(11, Memory_System::calculate_total_read_write_pages(LARGE_PAGE_SIZE));
  EXPECT_EQ(11, Memory_System::calculate_total_read_mostly_pages(LARGE_PAGE_SIZE));
  
  Logical_Core::group_size   = 13;
  EXPECT_EQ(13, Memory_System::calculate_total_read_write_pages(LARGE_PAGE_SIZE));
  EXPECT_EQ(13, Memory_System::calculate_total_read_mostly_pages(LARGE_PAGE_SIZE));
  
  Logical_Core::group_size   = 55;
  EXPECT_EQ(55, Memory_System::calculate_total_read_write_pages(LARGE_PAGE_SIZE));
  EXPECT_EQ(14, Memory_System::calculate_total_read_mostly_pages(LARGE_PAGE_SIZE));
  
  Logical_Core::group_size   = 59;
  EXPECT_EQ(59, Memory_System::calculate_total_read_write_pages(LARGE_PAGE_SIZE));
  EXPECT_EQ(15, Memory_System::calculate_total_read_mostly_pages(LARGE_PAGE_SIZE));
}

