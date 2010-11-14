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

/** A simple class used in the tests **/
class MyClass {
public:
  int foo() { return 42; }
};

/**
 * Test the simplest use case that a pointer was used in
 * a scope, gets properly registered, and then also properly
 * cleaned up from the registery after it is not used anymore.
 */
TEST(TrackedPointer, DirectUse) {
  int i = 42;
  
  // nothing done yet, so the pointer should not be registered
  ASSERT_EQ(0u, tracked_ptr<int>::pointers_on_stack());
  
  { // open new scope for the test
    tracked_ptr<int> i_p(&i);
    ASSERT_EQ(42, *i_p);
    
    // now the pointer is registered and valid for use
    ASSERT_EQ(1u, tracked_ptr<int>::pointers_on_stack());
    ASSERT_TRUE(i_p.is_valid());
  }
  
  // after closing the scope, everything should be cleaned up
  ASSERT_EQ(0u, tracked_ptr<int>::pointers_on_stack());
}

/**
 * Similar to the test before, but this time invalidate
 * all pointers in-between.
 */
TEST(TrackedPointer, DirectUseInvalidationInBetween) {
  int i = 42;
  
  ASSERT_EQ(0u, tracked_ptr<int>::pointers_on_stack());
  
  { // open new scope for the test
    tracked_ptr<int> i_p(&i);
    ASSERT_EQ(42, *i_p);
    
    // now the pointer is registered and valid for use
    ASSERT_EQ(1u, tracked_ptr<int>::pointers_on_stack());
    ASSERT_TRUE(i_p.is_valid());
    
    tracked_ptr<int>::invalidate_all_pointer();
    
    // now the pointer is invalidated and should not be used anymore
    ASSERT_EQ(1u, tracked_ptr<int>::pointers_on_stack());
    ASSERT_FALSE(i_p.is_valid());
  }
  
  // after closing the scope, everything should be cleaned up
  ASSERT_EQ(0u, tracked_ptr<int>::pointers_on_stack());
}

/**
 * Allow invalid pointers that are still on the stack
 */
TEST(TrackedPointer, InvalidationThenDestruction) {
  int i = 42;
  
  { // open new scope for the test
    tracked_ptr<int> i_p(&i);
    ASSERT_EQ(42, *i_p);
    
    ASSERT_TRUE(i_p.is_valid());
    
    tracked_ptr<int>::invalidate_all_pointer();
    
    ASSERT_EQ(1u, tracked_ptr<int>::pointers_on_stack());
    ASSERT_FALSE(i_p.is_valid());
  }

  ASSERT_EQ(0u, tracked_ptr<int>::pointers_on_stack());
  
  // Ensure subsequent use is still possible
  
  { // open new scope for the test
    tracked_ptr<int> i_p(&i);
    ASSERT_EQ(42, *i_p);
    
    ASSERT_TRUE(i_p.is_valid());
    
    tracked_ptr<int>::invalidate_all_pointer();
    
    ASSERT_EQ(1u, tracked_ptr<int>::pointers_on_stack());
    ASSERT_FALSE(i_p.is_valid());
  }
  
  
  // after closing the scope, everything should be cleaned up
  ASSERT_EQ(0u, tracked_ptr<int>::pointers_on_stack());
}

/**
 * The subsequent creation of a new valid pointer to the same
 * object needs to be allowed.
 */
TEST(TrackedPointer, CleanSeparationBetweenTrackedPointers) {
  int i = 42;
  
  { // open new scope for the test
    tracked_ptr<int> i_p(&i);
    ASSERT_EQ(42, *i_p);
    
    ASSERT_TRUE(i_p.is_valid());
    
    tracked_ptr<int>::invalidate_all_pointer();
    
    // now that one is invalid, however, we can obtain a new one
    ASSERT_EQ(1u, tracked_ptr<int>::pointers_on_stack());
    ASSERT_FALSE(i_p.is_valid());
    
    
    // obtain a new one
    tracked_ptr<int> i_p2(&i);
    ASSERT_EQ(42, *i_p2);
    
    ASSERT_TRUE(i_p2.is_valid());
    ASSERT_FALSE(i_p.is_valid()); // the old is unchanged
    
    // and both are still tracked
    ASSERT_EQ(2u, tracked_ptr<int>::pointers_on_stack()); 
    
    tracked_ptr<int>::invalidate_all_pointer();
    
    // now both need to be invalid
    ASSERT_FALSE(i_p.is_valid());
    ASSERT_FALSE(i_p2.is_valid());
    
    // and both are still tracked
    ASSERT_EQ(2u, tracked_ptr<int>::pointers_on_stack()); 
  }
  
  // after closing the scope, everything should be cleaned up
  ASSERT_EQ(0u, tracked_ptr<int>::pointers_on_stack());
}


/**
 * Make sure the use of an invalid pointer triggers an error.
 */
TEST(TrackedPointer, InvalidUseTriggersErrorArrow) {
  MyClass* bar = new MyClass();
  
  { // open new scope for the test
    tracked_ptr<MyClass> bar_p(bar);
    ASSERT_EQ(42, bar_p->foo());
        
    tracked_ptr<MyClass>::invalidate_all_pointer();
    
    ASSERT_DEATH(bar_p->foo(), "");
  }
  
  delete bar;
}

/**
 * Make sure the use of an invalid pointer triggers an error.
 */
TEST(TrackedPointer, InvalidUseTriggersErrorStar) {
  MyClass* bar = new MyClass();
  
  { // open new scope for the test
    tracked_ptr<MyClass> bar_p(bar);
    ASSERT_EQ(42, bar_p->foo());
    
    tracked_ptr<MyClass>::invalidate_all_pointer();
    
    ASSERT_DEATH((*bar_p).foo(), "");
  }
  
  delete bar;
}

/**
 * Testing the assignment semantics, it has to be possible to pass on
 * the tracked_ptr between variables without losing its semantics.
 */
TEST(TrackedPointer, AssignmentSemantics) {
  // make sure everything is clean
  EXPECT_EQ(0u, tracked_ptr<MyClass>::pointers_on_stack());
  EXPECT_EQ(0u, tracked_ptr<int>::pointers_on_stack());
  
  MyClass* bar = new MyClass();
  
  // this one is not yet initialized, so nothing should get registered
  tracked_ptr<MyClass> bar_p;
  
  EXPECT_EQ(0u, tracked_ptr<MyClass>::invalidate_all_pointer());
  EXPECT_FALSE(bar_p.is_valid()); // not initialize so it should also be invalid
  
  // now for convinence, we just assign the pointer, and it should be converted
  // implicitly as well as get registered correctly
  bar_p = bar;
  EXPECT_EQ(1u, tracked_ptr<MyClass>::pointers_on_stack());
  
  { // open new scope for the test
    
    // have a second pointer, this one should be tracked separatly I think (?)
    tracked_ptr<MyClass> bar_p2 = bar_p;
    EXPECT_EQ(2u, tracked_ptr<MyClass>::pointers_on_stack());
    EXPECT_TRUE(bar_p2.is_valid());
    
  } // and we clean things up immediately 
  EXPECT_EQ(1u, tracked_ptr<MyClass>::pointers_on_stack()); 
  
  EXPECT_TRUE(bar_p.is_valid()); // nothing should have invalidated it
  
  // lets do the same again
  { // open new scope for the test
    
    // have a second pointer, this one should be tracked separatly I think (?)
    tracked_ptr<MyClass> bar_p2 = bar_p;
    EXPECT_EQ(2u, tracked_ptr<MyClass>::pointers_on_stack());
    EXPECT_TRUE(bar_p2.is_valid());
    
    // but now we invalidate everything
    tracked_ptr<MyClass>::invalidate_all_pointer();
    
    EXPECT_FALSE(bar_p.is_valid());
    EXPECT_FALSE(bar_p2.is_valid());
  } // and we clean things up immediately 
  EXPECT_EQ(1u, tracked_ptr<MyClass>::pointers_on_stack()); 
  
  EXPECT_FALSE(bar_p.is_valid());
  
  // however, when we obtain a new one that should be fine again
  bar_p = bar;
  EXPECT_TRUE(bar_p.is_valid());
  
  // but still only a single object
  EXPECT_EQ(1u, tracked_ptr<MyClass>::pointers_on_stack()); 
  
  delete bar;
}


/**
 * Testing the assignment semantics, the first test was passing
 * stuff up the stack, don't know whether it makes a lot difference,
 * but just to be sure, lets test the other direction.
 */
TEST(TrackedPointer, AssignmentSemantics2) {
  // make sure everything is clean
  EXPECT_EQ(0u, tracked_ptr<MyClass>::pointers_on_stack());
  EXPECT_EQ(0u, tracked_ptr<int>::pointers_on_stack());
  
  MyClass* bar = new MyClass();
  
  // this one is not yet initialized, so nothing should get registered
  tracked_ptr<MyClass> bar_p;
  
  EXPECT_EQ(0u, tracked_ptr<MyClass>::invalidate_all_pointer());
  EXPECT_FALSE(bar_p.is_valid()); // not initialize so it should also be invalid
  
  { // open new scope for the test
    
    // have a second pointer, this one should be tracked separatly I think (?)
    tracked_ptr<MyClass> bar_p2(bar);
    EXPECT_EQ(1u, tracked_ptr<MyClass>::pointers_on_stack());
    EXPECT_TRUE(bar_p2.is_valid());
    
    // now pass it back to the first one
    bar_p = bar_p2;
    EXPECT_EQ(2u, tracked_ptr<MyClass>::pointers_on_stack());
    EXPECT_TRUE(bar_p.is_valid());
  } // and we clean things up immediately 
  EXPECT_EQ(1u, tracked_ptr<MyClass>::pointers_on_stack()); 
  
  EXPECT_TRUE(bar_p.is_valid()); // nothing should have invalidated it
    
  delete bar;
}


/**
 * Bascially a compilation test, to ensure that the interfaces
 * are correct and tracked_ptr<T> can be used substituting T*
 */
TEST(TrackedPointer, DesignTest) {
  MyClass* bar = new MyClass();

  EXPECT_EQ(42, bar->foo());
  EXPECT_EQ(42, (*bar).foo());
  
  tracked_ptr<MyClass> bar_p(bar);
  
  ASSERT_EQ(42, bar_p->foo());
  ASSERT_EQ(42, (*bar_p).foo());
  
  delete bar;
}

