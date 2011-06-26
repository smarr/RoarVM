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


# undef assert


extern void assert_failure(   const char* func, const char* file, const int line, const char* pred, const char* msg) __attribute__((noreturn));
extern void assert_eq_failure(const char* func, const char* file, const int line, const char* pred, const char* msg, void*, void*) __attribute__((noreturn));
extern void assert_eq_failure(const char* func, const char* file, const int line, const char* pred, const char* msg, int, int) __attribute__((noreturn));


// use this in verify:
# define assert_always(pred) assert_always_msg(pred, "")
# define assert_always_msg(pred, msg) ((pred) ? ({}) : assert_failure(__func__, __FILE__, __LINE__, #pred, msg))
# define assert_always_eq(a, b) \
  ((a) == (b) ? ({}) : assert_eq_failure(__func__, __FILE__, __LINE__, #a "  ==  " #b, "", (a), (b)))


# define assert_message(pred, msg) \
  assert_always_msg((!check_assertions || (pred)), msg)

# define assert(pred) assert_message(pred, "")

# define assert_on_main() assert_eq(Logical_Core::my_rank(), Logical_Core::main_rank, "main?")




# define assert_eq(a, b, msg) \
  (!check_assertions ||  (a) == (b) ?  ({}) : assert_eq_failure(__func__, __FILE__, __LINE__, #a "  ==  " #b, msg, (a), (b)))

# define fatal(msg) assert_always_msg(0, "Fatal: " msg)
# define unimplemented() assert_message(0, "Unimplemented")


#define untested() {unt("untested", __func__); }

# define unimpExt() {unte("unimplemented external", __func__); }

extern void breakpoint();
extern void unt(const char*, const char*);
extern void unte(const char*, const char*);


#define unimp_608() (fatal("stub used during June 2008 reorg\n"), 0)

#define assert_active_process_not_nil() \
  if ( \
    The_Squeak_Interpreter()->schedulerPointer_obj()->fetchPointer(Object_Indices::ActiveProcessIndex) == The_Squeak_Interpreter()->roots.nilObj ) \
  assert_failure(__func__, __FILE__, __LINE__, "Processor activeProcess must not be nil", "activeProcess is nil"); \
  else 

