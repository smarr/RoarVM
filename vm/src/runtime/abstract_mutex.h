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


// Linux compiler doesn't like templates

void trace_mutex_evt(int, const char*);

class OS_Mutex_Interface {
private:
  struct {
    int recursion_count;
    bool is_held;
    u_int64 acq_cycles;
    u_int64 rel_cycles;
  } logical_core_local;

  struct {
    int* holder;    // rank of the holding process
    OS_Interface::Mutex* os_mutex;  // mutex provided by the underlying system
  } logical_core_global;

public:
  OS_Mutex_Interface() {
    logical_core_local.recursion_count = 0;
    logical_core_local.is_held = false;
    logical_core_local.acq_cycles = 0;
    logical_core_local.rel_cycles = 0;

    logical_core_global.holder = NULL;
    logical_core_global.os_mutex = NULL;
  }

  void initialize_globals() {
    logical_core_global.os_mutex = (OS_Interface::Mutex*)Memory_Semantics::shared_malloc(sizeof(OS_Interface::Mutex));
    OS_Interface::mutex_init(logical_core_global.os_mutex);

    logical_core_global.holder = (int*)OS_Interface::malloc_uncacheable_shared(sizeof(int), sizeof(int));
    *logical_core_global.holder = -1;
  }

  inline bool is_initialized() {
    return logical_core_global.holder != NULL;
  }

  inline bool is_held() {
    return logical_core_local.is_held;
  }

  inline int get_holder() {
    return *logical_core_global.holder;
  }

  inline void set_holder(int holder) {
    *logical_core_global.holder = holder;
  }

  inline void aquire() {
    logical_core_local.is_held = true;
  }

  inline void release() {
    logical_core_local.is_held = false;
  }

  inline int check_and_inc_recursion_depth() {
    return logical_core_local.recursion_count++;
  }

  inline int dec_and_check_recursion_depth() {
    return --logical_core_local.recursion_count;
  }

  inline void add_acq_cycles(u_int64 inc) {
    logical_core_local.acq_cycles += inc;
  }

  inline u_int64 get_and_reset_acq_cycles() {
    u_int64 result = logical_core_local.acq_cycles;
    logical_core_local.acq_cycles = 0;
    return result;
  }

  inline void add_rel_cycles(u_int64 inc) {
    logical_core_local.rel_cycles += inc;
  }

  inline u_int64 get_and_reset_rel_cycles() {
    u_int64 result = logical_core_local.rel_cycles;
    logical_core_local.rel_cycles = 0;
    return result;
  }

  void print_stats() {
    lprintf( "acq cycles = %lld, rel cycles = %lld\n",
            logical_core_local.acq_cycles, logical_core_local.rel_cycles);
  }

  inline bool try_lock() {
    return OS_Interface::mutex_trylock(logical_core_global.os_mutex);
  }

  inline int unlock() {
    return OS_Interface::mutex_unlock(logical_core_global.os_mutex);
  }
};


# define Define_RVM_Mutex(Class_Name, Actions, acquire_ID, release_ID) \
class Class_Name { \
private: \
  Safepoint_Ability sa; \
  friend class Actions; \
  const char* why; \
  \
public: \
  static bool is_held()  { return Actions::is_held(); } \
  static void assert_held()  { assert(!Actions::is_initialized() || is_held()); } \
  Class_Name(const char* w = "") : sa(Safepoint_Ability::is_interpreter_able()) /* must be true while waiting to acquire, otherwise could deadlock */ { \
    \
    why = w; \
    u_int64 start = OS_Interface::get_cycle_count(); \
    \
    if (acquire_ID) trace_mutex_evt(acquire_ID, why);\
    \
    OS_Mutex_Interface* mutex = Actions::get_mutex(); \
    /* Tricky; some mutexes may recurse since they have a receive_and_handle_one_message. \
       If so, need to acquire mutex even though are recursing, so delay increment of recursion_count \
       However, clients of this macro must also manipulate recursion_count: see them! */ \
     if (mutex->check_and_inc_recursion_depth() == 0  &&  Actions::is_initialized()) { \
      Actions::acquire_action(why); \
      sa.be_unable(); \
      mutex->aquire(); \
      if (acquire_ID) trace_mutex_evt(100+acquire_ID, why);\
    } \
    mutex->add_acq_cycles(OS_Interface::get_cycle_count() - start); \
  } \
  ~Class_Name() { \
    \
    u_int64 start = OS_Interface::get_cycle_count(); \
\
    if (release_ID) trace_mutex_evt(release_ID, why);\
    \
    OS_Mutex_Interface* mutex = Actions::get_mutex(); \
    if (mutex->dec_and_check_recursion_depth() <= 0  &&  Actions::is_initialized()) { \
      if (release_ID) trace_mutex_evt(100+release_ID, why);\
      /* assert_always(mutex->local.recursion_count == 0); */ \
      Actions::release_action(why); \
      mutex->release(); \
    } \
    mutex->add_rel_cycles(OS_Interface::get_cycle_count() - start); \
  } \
};

