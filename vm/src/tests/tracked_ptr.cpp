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


# if !On_Tilera

# define Track_OnStackPointer 1  /* Enforce this here for the tests */

# include <gtest/gtest.h>

# include "headers.h"


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
  
  // this one is not yet initialized, but it already gets registered
  tracked_ptr<MyClass> bar_p;
  
  EXPECT_EQ(1u, tracked_ptr<MyClass>::pointers_on_stack());
  EXPECT_FALSE(bar_p.is_valid()); // not initialize so it should also be invalid
  
  // now for convinence, we just assign the pointer, and it should be converted
  // implicitly
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
    
    // have a second tracked pointer
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
  
  // this one is not yet initialized
  tracked_ptr<MyClass> bar_p;
  
  EXPECT_EQ(1u, tracked_ptr<MyClass>::pointers_on_stack());
  EXPECT_FALSE(bar_p.is_valid()); // not initialize so it should be invalid
  
  { // open new scope for the test
    
    // have a second tracked pointer
    tracked_ptr<MyClass> bar_p2(bar);
    EXPECT_EQ(2u, tracked_ptr<MyClass>::pointers_on_stack());
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
  
  // Can I get the wrapped pointer out of the address in the same way
  // I can get it from a standard pointer?
  ASSERT_EQ(&(*bar), &(*bar_p));
  ASSERT_EQ(&(*bar), bar);
  ASSERT_EQ(&(*bar_p), bar);
  ASSERT_TRUE(&(*bar_p) == bar);
  
  delete bar;
}

/**
 * Two tracked_ptr compared for equality with each other should be equal
 * if the wrapped pointer are equal.
 * They should be different, when their wrapped pointer are different.
 *
 * REM: This is no < or > comparison, since this is needed in the contain
 *      i.e. in the registry we are currently using...
 */
TEST(TrackedPointer, Equality) {
  MyClass* bar = new MyClass();
  MyClass* foo = new MyClass();
  
  // two wrapper for the same pointer
  tracked_ptr<MyClass> bar_p(bar);
  tracked_ptr<MyClass> bar_p2(bar);
  
  // a wrapper for another pointer
  tracked_ptr<MyClass> foo_p(foo);
  
  // this is the basic assumption, independent of any test-framework specifics
  ASSERT_TRUE (bar_p == bar_p2);
  ASSERT_FALSE(bar_p != bar_p2);
  
  ASSERT_FALSE(bar_p == foo_p);
  ASSERT_TRUE (bar_p != foo_p);
  
  // the EQ and NE macros introduce const quantifiers a long the way
  // so that has to work, too.
  ASSERT_EQ(bar_p, bar_p2);
  ASSERT_NE(bar_p, foo_p);
  
  delete bar;
  delete foo;
}

/**
 * A tracked_ptr compared with T or void* for equality with each other should be equal
 * if the pointer are equal.
 * They should be different, when their pointer are different.
 *
 * REM: This is no < or > comparison, since this is needed in the contain
 *      i.e. in the registry we are currently using...
 */
TEST(TrackedPointer, EqualityWithPointer) {
  MyClass* bar = new MyClass();
  MyClass* foo = new MyClass();
  
  tracked_ptr<MyClass> bar_p(bar);
  
  // a wrapper for another pointer
  tracked_ptr<MyClass> foo_p(foo);
  
  // this is the basic assumption, independent of any test-framework specifics
  ASSERT_TRUE (bar_p == bar);
  ASSERT_FALSE(bar_p != bar);  
  ASSERT_FALSE(bar_p == foo);
  ASSERT_TRUE (bar_p != foo_p);
  
  // the EQ and NE macros introduce const quantifiers a long the way
  // so that has to work, too.
  ASSERT_EQ(bar_p, bar);
  ASSERT_NE(bar_p, foo);
  ASSERT_EQ(foo_p, foo);
  ASSERT_NE(foo_p, bar);
  
  // and now make sure everything works with a void* and NULL, too
  //void* bar_v = bar;

  //ASSERT_EQ(bar_p, bar_v);
  ASSERT_NE(bar_p, NULL);
  //ASSERT_NE(foo_p, bar_v);

  foo_p = NULL;
  ASSERT_EQ(foo_p, NULL);
  
  delete bar;
  delete foo;
}

/**
 * The getter function 
 */
TEST(TrackedPointer, Getter) {
  MyClass* bar = new MyClass();
  MyClass* foo = new MyClass();
  
  // two wrapper for the same pointer
  tracked_ptr<MyClass> bar_p(bar);
  tracked_ptr<MyClass> bar_p2(bar);
  
  // a wrapper for another pointer
  tracked_ptr<MyClass> foo_p(foo);
  
  ASSERT_EQ(bar, bar_p.get());
  ASSERT_EQ(bar, bar_p2.get());
  
  ASSERT_EQ(foo, foo_p.get());
  
  delete bar;
  delete foo;
}

/**
 * This should implicitly lead to an error most of the time,
 * but make sure it also errors explicitly. Fail Early!
 */
TEST(TrackedPointer, NullPointerAreInvalidForDeref) {
  MyClass* bar = NULL;

  tracked_ptr<MyClass> bar_p(bar);

  ASSERT_DEATH((*bar_p).foo(), "");
  
  ASSERT_DEATH(bar_p->foo(), "");
    
  delete bar;
}

/**
 * Comparison with boolean should work equally as with its pointer.
 * The question is, should I take validity into account?
 * I think, yes.
 */
TEST(TrackedPointer, ComparisonWithBoolean) {
  MyClass* bar = new MyClass();
  
  tracked_ptr<MyClass> bar_p(bar);
  tracked_ptr<MyClass> bar_p2;
  
  // the simple one with AND
  ASSERT_TRUE(true && bar);    // that is what is expected
  ASSERT_TRUE(true && bar_p);
  
  // ok, invalidate the pointer
  tracked_ptr<MyClass>::invalidate_all_pointer();
  
  EXPECT_FALSE(bar_p.is_valid());
  ASSERT_FALSE(true && bar_p);
  
  // make a new one that is valid
  bar_p = bar;
  EXPECT_TRUE(bar_p.is_valid());
  
  // try it with an assignment, its one of the use-cases in RoarVM
  ASSERT_TRUE(true && (bar_p2 = bar_p));
  
  // and just to make sure that the false case is covered as well
  bar_p = NULL;
  ASSERT_FALSE(true && bar_p);
  
  delete bar;
}

/**
 * Ensure that the short-circuiting of && works properly.
 */
tracked_ptr<MyClass> _helperThatShouldBeNeverCalled(bool& wasNeverCalled, MyClass* bar) {
  wasNeverCalled = false;
  return tracked_ptr<MyClass>(bar);
}
bool _helperThatShouldBeCalledAlways(bool& wasCalled) {
  wasCalled = true;
  return false;
}
TEST(TrackedPointer, ShortCircuiting) {
  MyClass* bar = new MyClass();
  
  bool shouldStayTrue = true;
  bool shouldBecomeTrue = false;
  
  bool testResult = _helperThatShouldBeCalledAlways(shouldBecomeTrue) && _helperThatShouldBeNeverCalled(shouldStayTrue, bar);
  ASSERT_FALSE(testResult);
  ASSERT_TRUE(shouldStayTrue);
  ASSERT_TRUE(shouldBecomeTrue);
}

/**
 * Do casts do the right thing?
 */
TEST(TrackedPointer, Casts) {
  MyClass* bar = new MyClass();
  
  tracked_ptr<MyClass> bar_p = (tracked_ptr<MyClass>)bar;
  
  ASSERT_EQ(bar_p, bar);
  ASSERT_NE(bar_p, (tracked_ptr<MyClass>)NULL);
  
  ASSERT_EQ(1u, tracked_ptr<MyClass>::pointers_on_stack());
}

/**
 * The ternary operator should convert the tracked pointer to a bool.
 */ 
TEST(TrackedPointer, TernaryOperator) {
  MyClass* bar = new MyClass();
  
  tracked_ptr<MyClass> bar_p = (tracked_ptr<MyClass>)bar;
  
  ASSERT_TRUE(bar_p ? true : false);
  
  bar_p = NULL;
  ASSERT_FALSE(bar_p ? true : false);
}

TEST(TrackedPointer, ReferenceOperator) {
  // compiler check
  int i = 0;
  int* i_p = &i;
  // compiler check done
  
  MyClass* bar = new MyClass();
  
  tracked_ptr<MyClass> bar_p = (tracked_ptr<MyClass>)bar;
  MyClass* const * bar_pp;
  
  ASSERT_DEATH(bar_pp = &bar_p, "");  //We want that to fail for the moment!
  
  // REM: if we don't want it to fail, then the following should hold:
  //  ASSERT_EQ(*bar_pp, bar);
  //  ASSERT_NE((MyClass*)NULL, bar);
}

TEST(TrackedPointer, CastVoidP) {
  MyClass* bar = new MyClass();
  
  tracked_ptr<MyClass> bar_p = (tracked_ptr<MyClass>)bar;
  void* bar_v = (void*)bar_p;
  
  ASSERT_EQ(bar_v, bar);
  ASSERT_NE((void*)NULL, bar_v);
}

TEST(TrackedPointer, CastToContainedPointer) {
  MyClass* bar = new MyClass();
  
  tracked_ptr<MyClass> bar_p = (tracked_ptr<MyClass>)bar;
  MyClass* bar2 = (MyClass*)bar_p;
  MyClass* bar3 = bar_p;
  
  
  ASSERT_EQ(bar2, bar);
  ASSERT_NE((MyClass*)NULL, bar2);
}

/**
 * It should be possible to catch somehow the invalidation of ``this``
 * during a method call.
 */
class InvalidationDuringCall {
public:
  volatile bool testField1;
  volatile bool testField2;
  
  InvalidationDuringCall() : testField1(true), testField2(true) {}
  
  void someMethodChangingFields() {
    testField1 = not testField1 or testField2;
    this->testField2 = not this->testField2;
  }
  
  void somethingComplexWhichWillProvokeInvalidationInTheMiddle() {
    testField1 = true; // that should work with a valid this
    
    tracked_ptr<InvalidationDuringCall>::invalidate_all_pointer();
    
    // well, and that should fail. Not sure yet how to achieve that
    testField2 = true;
  }
  
  inline InvalidationDuringCall* operator-> () {
    return this;
  }
};
// currently diabled since it does not seem to be possible to capture this and 
// do magic with it in the method body
TEST(TrackedPointer, DISABLED_InvalidationDuringCall) {
  tracked_ptr<InvalidationDuringCall> t = (tracked_ptr<InvalidationDuringCall>)new InvalidationDuringCall();
  
  t->someMethodChangingFields();
  t->somethingComplexWhichWillProvokeInvalidationInTheMiddle();
  t->someMethodChangingFields();
}

# endif // !On_Tilera

