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


#include "tracked_ptr.h"

#include <gtest/gtest.h>

/**
 * Test the simplest use case that a pointer was used in
 * a scope, gets properly registered, and then also properly
 * cleaned up from the registery after it is not used anymore.
 */
TEST(TrackedPointer, DirectUse) {
  int i = 42;
  
  // nothing done yet, so the pointer should not be registered
  ASSERT_FALSE(tracked_ptr<int>::is_registered(&i));
  
  // and also, it should be invalid for use
  ASSERT_FALSE(tracked_ptr<int>::is_valid(&i));
  
  { // open new scope for the test
    tracked_ptr<int> i_p(&i);
    ASSERT_EQ(42, *i_p);
    
    // now the pointer is registered and valid for use
    ASSERT_TRUE(tracked_ptr<int>::is_registered(&i));
    ASSERT_TRUE(tracked_ptr<int>::is_valid(&i));
  }
  
  // after closing the scope, everything should be cleaned up
  ASSERT_FALSE(tracked_ptr<int>::is_registered(&i));
  ASSERT_FALSE(tracked_ptr<int>::is_valid(&i));
}

/**
 * Similar to the test before, but this time invalidate
 * all pointers in-between.
 */
TEST(TrackedPointer, DirectUseInvalidationInBetween) {
  int i = 42;
  
  { // open new scope for the test
    tracked_ptr<int> i_p(&i);
    ASSERT_EQ(42, *i_p);
    
    // now the pointer is registered and valid for use
    ASSERT_TRUE(tracked_ptr<int>::is_registered(&i));
    ASSERT_TRUE(tracked_ptr<int>::is_valid(&i));
    
    tracked_ptr<int>::invalidate_all_pointer();
    
    // now the pointer is registered and valid for use
    ASSERT_TRUE(tracked_ptr<int>::is_registered(&i));
    ASSERT_FALSE(tracked_ptr<int>::is_valid(&i));
  }
  
  // after closing the scope, everything should be cleaned up
  ASSERT_FALSE(tracked_ptr<int>::is_registered(&i));
  ASSERT_FALSE(tracked_ptr<int>::is_valid(&i));
}

/**
 * Bascially a compilation test, to ensure that the interfaces
 * are correct and tracked_ptr<T> can be used substituting T*
 */
class MyClass {
public:
  int foo() { return 42; }
};

TEST(TrackedPointer, DesignTest) {
  MyClass* bar = new MyClass();

  EXPECT_EQ(42, bar->foo());
  EXPECT_EQ(42, (*bar).foo());
  
  tracked_ptr<MyClass> bar_p(bar);
  
  ASSERT_EQ(42, bar_p->foo());
  ASSERT_EQ(42, (*bar_p).foo());
}

