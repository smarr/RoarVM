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


# if On_Tilera_With_GCC

# include <arch/atomic.h>
# include <tmc/spin.h>
# include <tmc/mspace.h>
# include <tmc/task.h>
# include <tmc/cpus.h>

/* The page size values correspond to the systems standard of Tilera MDE 3.0.1
   PAGE_SIZE       == getpagesize() 
   LARGE_PAGE_SIZE == tmc_alloc_get_huge_pagesize() */
# define PAGE_SIZE 64 * 1024
# define LARGE_PAGE_SIZE 16 * Mega

class TMC_OS_Interface : public Abstract_OS_Interface {
public:
  
  static inline void abort() __attribute__((noreturn))  { ::abort(); }
  static inline void die(const char* err_msg) __attribute__((noreturn)) { tmc_task_die(err_msg); }
  static inline void exit() __attribute__((noreturn)) {
    // set_sim_tracing(SIM_TRACE_NONE);
    profiler_disable();
    tmc_task_terminate_app();
    ::exit(0);
  }
  
  static inline void initialize() {}
  
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

  
  typedef tmc_spin_mutex_t Mutex;
  
  static inline void mutex_init(Mutex* mutex, const void* = NULL) {
    tmc_spin_mutex_init(mutex);
  }
  
  static inline void mutex_destruct(Mutex* mutex) {}
  
  static inline int mutex_lock(Mutex* mutex) {
    tmc_spin_mutex_lock(mutex);
    return 0;
  }
  
  static inline bool mutex_trylock(Mutex* mutex) {
    return 0 == tmc_spin_mutex_trylock(mutex);
  }
  
  static inline int mutex_unlock(Mutex* mutex) {
    tmc_spin_mutex_unlock(mutex);
    return 0;
  }
  
  static inline int atomic_fetch_and_add(int* mem, int increment) {
    return atomic_add(mem, increment);
  }
  
  /**
   * Atomically compare the memory location with the old value, and 
   * if they are equal set the new value and return true, false otherwise.
   */
  static inline bool atomic_compare_and_swap(int* ptr, int old_value, int new_value) {
    return atomic_bool_compare_and_exchange(ptr, old_value, new_value);
  }
  
  /**
   * Atomically compare the memory location with the old value, and 
   * if they are equal set the new value, otherwise don't set anything.
   * 
   * Returns the initial value at ptr.
   */
  static inline int atomic_compare_and_swap_val(int* ptr, int old_value, int new_value) {
    return atomic_val_compare_and_exchange(ptr, old_value, new_value);
  }
  
    
  static inline uint32_t leading_zeros(uint32_t x)    { return __insn_clz(x);  }
  static inline uint32_t population_count(uint32_t x) { return __insn_pcnt(x); }
  
  // About tmc_cmem_init:
  // It would be more sensible to call it indirectly from main, but static constructors
  // run before main, so that won't work.
  // It might seem better to only call it on the first allocation, which would require this code
  // to keep and test a static flag. However, the Tilera documentation implies that the tmc_cmem_init
  // routine itself handles this case, so why complicate our code to do it, too?
  // See tile64/doc/html/application_libraries_reference/index.html
  
  
  // Named rvm_?alloc_shared since Tilera headers are using macros with the same name
  static inline void* rvm_malloc_shared(size_t sz) {
    rvm_malloc_shared_init(); // called by static ctor, so need to do it here, sigh
    return tmc_cmem_malloc(sz);
  }
  static inline void* rvm_calloc_shared(size_t num_members, size_t mem_size)  {
    rvm_malloc_shared_init(); // called by static ctor, so need to do it here, sigh
    return tmc_cmem_calloc(num_members, mem_size);
  }
private:
  static inline void rvm_malloc_shared_init() {  
    abort_if_error("tmc_cmem_init failed", tmc_cmem_init(0)); 
  }
public:
  
  typedef tmc_mspace OS_Heap;
  
  static inline void* rvm_memalign(int al, int sz)               { return tmc_cmem_memalign(al, sz); }
  static inline void* rvm_memalign_shared(OS_Heap heap, int al, int sz) { return tmc_mspace_memalign(heap, al, sz); }
  static        void* malloc_uncacheable_shared(int alignment, int size){ return rvm_memalign(alignment, size); }
  static inline void  invalidate_mem(void* ptr, size_t size)     { tmc_mem_inv(ptr, size); }
  static inline void  mem_flush(void* ptr, size_t size)          { tmc_mem_flush(ptr, size); }
  static inline void  mem_fence()                                { tmc_mem_fence(); }
  static inline int   mem_create_heap_if_on_Tilera(OS_Heap* heap, bool replicate) {
    tmc_alloc_t flags = TMC_ALLOC_INIT;
    if (replicate)
      tmc_alloc_set_home(&flags, TMC_ALLOC_HOME_INCOHERENT);
    
    *heap = tmc_mspace_create_special(0, 0, &flags);
    return 0;
  }
  
  static inline int get_process_rank() { return tmc_cpus_get_task_current_cpu(tmc_task_gettid()); }
  
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
