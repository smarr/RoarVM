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



// When debugging, if you need to catch a store into the object heap, you can:
// 1. Make sure Compile_Debug_Store_Checks is set to 1 in rvm_config, and
// 2. Put your check into debug_store_check, where it says: "your code here"
// -- dmu 10/1/10

# define IMAGE_READING_DEBUG_STORE_CHECK(x, y) DEBUG_STORE_CHECK(x, y)
# define IMAGE_READING_DEBUG_MULTIMOVE_CHECK(dstp, srcp, n) DEBUG_MULTIMOVE_CHECK(dstp, srcp, n)

# if ! Compile_Debug_Store_Checks

# define DEBUG_STORE_CHECK(x, y)
# define DEBUG_MULTISTORE_CHECK(dst, src, n)
# define DEBUG_MULTIMOVE_CHECK(dst, src, n)

# else

# define DEBUG_STORE_CHECK(x, y)                Debug_Store_Checks::debug_store_check(x, y)
# define DEBUG_MULTISTORE_CHECK(dstp, src, n)   Debug_Store_Checks::debug_multistore_check(dstp, src, n)
# define DEBUG_MULTIMOVE_CHECK(dstp, srcp, n)   Debug_Store_Checks::debug_multimove_check(dstp, srcp, n)



class Debug_Store_Checks {
  public:
  
  static void debug_store_check(const oop_int_t* addr, oop_int_t contents) {
    // your code here
  }
  
  
  static void debug_store_check(const Oop* addr, Oop contents) { debug_store_check((oop_int_t*)addr, contents.bits()); }
  
  static void debug_store_check(const Object** addr, Object* contents) { debug_store_check((oop_int_t*)addr, (oop_int_t)contents); }
  
  
  static void debug_multistore_check(const oop_int_t* addr, oop_int_t src, int n) {
    for (int i = 0;  i < n;  ++i)
      debug_store_check(&addr[i], src);
  }
  
  static void debug_multistore_check(const Oop* addr, Oop src, int n) {
    debug_multistore_check((oop_int_t*)addr, src.bits(), n);
  }
  
  static void debug_multimove_check(const void* dstp, const void* srcp, int n) {
    for (int i = 0;  i < n;  ++i)
      debug_store_check(&((Oop*)dstp)[i], ((Oop*)srcp)[i]);
  }
  
};



# endif
