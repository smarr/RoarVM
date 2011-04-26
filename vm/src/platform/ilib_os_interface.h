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


# if On_Tilera

# include <atomic.h>

class ILib_OS_Interface : public Abstract_OS_Interface {
public:
  
  static inline void abort()                  { ilib_abort(); }
  static inline void die(const char* err_msg) { ilib_die(err_msg); }
  static inline void exit() {
    // set_sim_tracing(SIM_TRACE_NONE);
    profiler_disable();
    ilib_terminate();
    ::exit(0);
  }
  
  static inline void initialize() {
    ilib_init();
  }
  
  static inline void ensure_Time_Machine_backs_up_run_directory() {}
  
  
  static inline void profiler_enable()  { sim_profiler_enable();  }
  static inline void profiler_disable() { sim_profiler_disable(); }
  static inline void profiler_clear()   { sim_profiler_clear();   }
  static inline void sim_end_tracing()  { sim_set_tracing(SIM_TRACE_NONE); };
  
  # if 0
    typedef u_int64 get_cycle_count_quickly_t;
    # define GET_CYCLE_COUNT_QUICKLY OS_Interface::get_cycle_count
    # define GET_CYCLE_COUNT_QUICKLY_FMT "%lld"
  # else
    typedef uint32_t get_cycle_count_quickly_t;
    # define GET_CYCLE_COUNT_QUICKLY  get_cycle_count_low
    # define GET_CYCLE_COUNT_QUICKLY_FMT "%ld"
  # endif
  static inline u_int64 get_cycle_count() { return ::get_cycle_count(); }

  
  typedef ilibMutex Mutex;
  
  static inline void mutex_init(Mutex* mutex, const void* = NULL) {
    ilib_mutex_init(mutex);
  }
  
  static inline void mutex_destruct(Mutex* mutex) {
    ilib_mutex_destroy(mutex);
  }
  
  static inline int mutex_lock(Mutex* mutex) {
    return ilib_mutex_lock(mutex);
  }
  
  static inline int mutex_trylock(Mutex* mutex) {
    return ilib_mutex_trylock(mutex);
  }
  
  static inline int mutex_unlock(Mutex* mutex) {
    return ilib_mutex_unlock(mutex);
  }
  
  static inline int atomic_fetch_and_add(int* mem, int increment) {
    return atomic_add_val(mem, increment);
  }
  
  /**
   * Atomically compare the memory location with the old value, and 
   * if they are equal set the new value and return true, false otherwise.
   */
  static inline bool atomic_compare_and_swap(int* ptr, int old_value, int new_value) {
    return (0 == atomic_compare_and_exchange_bool_acq(ptr, new_value, old_value)); // Not sure whether that is stable, this API is unintuitive for me, got it wrong twice!! make sure the test cases are rerun on new lib versions
  }
  
  /**
   * Atomically compare the memory location with the old value, and 
   * if they are equal set the new value, otherwise don't set anything.
   * 
   * Returns the initial value at ptr.
   */
  static inline int atomic_compare_and_swap_val(int* ptr, int old_value, int new_value) {
    return atomic_compare_and_exchange_val_acq(ptr, new_value, old_value);
  }
  
    
  static inline uint32_t leading_zeros(uint32_t x)    { return __insn_clz(x);  }
  static inline uint32_t population_count(uint32_t x) { return __insn_pcnt(x); }
  
# if Use_CMem
  // Named rvm_?alloc_shared since Tilera headers are using macros with the same name
  static inline void* rvm_malloc_shared(size_t sz) {
    return tmc_cmem_malloc(sz);
  }
  static inline void* rvm_calloc_shared(size_t num_members, size_t mem_size)  {
    return tmc_cmem_calloc(num_members, mem_size);
  }
# else
  static inline void* rvm_malloc_shared(size_t sz) {
    return malloc_shared(sz);
  }
  static inline void* rvm_calloc_shared(size_t num_members, size_t mem_size)  {
    return calloc_shared(num_members, mem_size);
  }
# endif
  
  typedef ilibHeap OS_Heap;
  
  static inline void* rvm_memalign(int al, int sz) { return memalign(al, sz); }
  static inline void* rvm_memalign(OS_Heap heap, int al, int sz) { return ilib_mem_memalign_heap(heap, al, sz); }
  static        void* malloc_in_mem(int alignment, int size);
  static inline void  invalidate_mem(void* ptr, size_t size) { ilib_mem_invalidate(ptr, size); }
  static inline void  mem_flush(void* ptr, size_t size) { ilib_mem_flush(ptr, size); }
  static inline void  mem_fence() { ilib_mem_fence(); }
  static inline int   mem_create_heap_if_on_Tilera(OS_Heap* heap, bool replicate) {
    return ilib_mem_create_heap(ILIB_MEM_SHARED | (replicate ? ILIB_MEM_USER_MANAGED : 0), heap);
  }
  
  static inline int get_process_rank() { return ilib_group_rank(ILIB_GROUP_SIBLINGS); }
  
  static void start_processes(void (*helper_core_main)(), char* argv[]);
  
  static int abort_if_error(const char*, int); 

  /**
  * this is a local spin, it avoids putting any memory presure
  * on the tile network while waiting for a few instruction
  * busy-local-only-loop
  */
  static inline void yield_or_spin_a_bit() {
    float a = 0.0;
    for (int i = 0;  i < 50;  i++)  a += i;
  }

};

# endif
