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


# if !On_Tilera

# define PAGE_SIZE 64 * 1024
# define LARGE_PAGE_SIZE 16 * Mega

# define MAP_CACHE_INCOHERENT 0

# if On_Intel_Linux
  # include <pthread.h>
  # include <sys/gmon.h>
  # define pthread_yield_np pthread_yield
# endif

#include <err.h>

class Interprocess_Allocator;

class POSIX_OS_Interface : public Abstract_OS_Interface {
public:
  
  static inline void abort() __attribute__((noreturn));
  static inline void die(const char* err_msg) __attribute__((noreturn)) {
    warnx("%s", err_msg);
    abort();
  }
  static inline void exit() __attribute__((noreturn));
  static inline void breakpoint() {
    if (Include_Debugging_Code) {
      /* asm ("int 3"); */
      raise(SIGTRAP);
    }
  }
  
  static inline void initialize();  
  static void ensure_Time_Machine_backs_up_run_directory() {}

  static inline void profiler_enable()  {}
  static inline void profiler_disable() {}
  static inline void profiler_clear()   {}
  static inline void sim_end_tracing()  {}
  
  
  typedef int get_cycle_count_quickly_t;
  # define GET_CYCLE_COUNT_QUICKLY  OS_Interface::dummy_get_cycle_count
  # define GET_CYCLE_COUNT_QUICKLY_FMT "%ld"
  static inline int dummy_get_cycle_count() { return 0; }
  static inline u_int64 get_cycle_count() {
    uint64_t result;
    
    if (Count_Cycles) {
      asm volatile("rdtsc" : "=A" (result));
      return result;
    }
    else
      return 0;
  }
  
  static bool AmIBeingDebugged() { return false; }
  
  
  
# if Omit_PThread_Locks
  
  typedef int Mutex;
  static inline void mutex_init(Mutex*) {}
  static inline void mutex_init_for_cross_process_use(Mutex* mutex) {}
  static inline void mutex_destruct(Mutex*)    {}
  static inline int  mutex_lock(Mutex*)        { return 0; }
  static inline bool mutex_trylock(Mutex*)     { return false; }
  static inline int  mutex_unlock(Mutex*)      { return 0; }
  
# elif Use_Spin_Locks && !On_Apple 
  
  typedef pthread_spinlock_t Mutex;
  
  static inline void mutex_init(Mutex* mutex, const void* _ = NULL) {
    pthread_spin_init(mutex, PTHREAD_PROCESS_PRIVATE);
  }
  
  static inline void mutex_init_for_cross_process_use(Mutex* mutex) {
    pthread_spin_init(mutex, PTHREAD_PROCESS_SHARED);
  }
  
  static inline void mutex_destruct(Mutex* mutex) {
    pthread_spin_destroy(mutex);
  }
  
  static inline int mutex_lock(Mutex* mutex) {
    return pthread_spin_lock(mutex);
  }
  
  static inline bool mutex_trylock(Mutex* mutex) {
    return 0 == pthread_spin_trylock(mutex);
  }
  
  static inline int mutex_unlock(Mutex* mutex) {
    return pthread_spin_unlock(mutex);
  }
  
# else

  typedef pthread_mutex_t Mutex;
  
  /** Initializes a normal/standard platform mutex. */
  static inline void mutex_init(Mutex* mutex) {
    pthread_mutex_init(mutex, NULL);
  }
  
  static inline void mutex_destruct(Mutex* mutex) {
    pthread_mutex_destroy(mutex);
  }
  
  static inline int mutex_lock(Mutex* mutex) {
    return pthread_mutex_lock(mutex);
  }
  
  static inline bool mutex_trylock(Mutex* mutex) {
    return 0 == pthread_mutex_trylock(mutex);
  }
  
  static inline int mutex_unlock(Mutex* mutex) {
    return pthread_mutex_unlock(mutex);
  }
  
  /** Initializes a mutex which explicitly supports cross-process usage. */
  static inline void mutex_init_for_cross_process_use(Mutex* mutex) {
    pthread_mutexattr_t process_shared_attr;
    pthread_mutexattr_init(&process_shared_attr);
    pthread_mutexattr_setpshared(&process_shared_attr, PTHREAD_PROCESS_SHARED);
    
    if (0 != pthread_mutex_init(mutex, &process_shared_attr)) {
      // TODO: do it properly
      perror("Creating a mutex for cross-process use failed.");
    }
  }

# endif // Omit_PThread_Locks elif Use_Spin_Locks
  
  static inline int atomic_fetch_and_add(int* mem, int increment) {
    return __sync_fetch_and_add(mem, increment);
  }
  
  /**
   * Atomically compare the memory location with the old value, and 
   * if they are equal set the new value and return true, false otherwise.
   */
  static inline bool atomic_compare_and_swap(int* ptr, int old_value, int new_value) {
    return __sync_bool_compare_and_swap(ptr, old_value, new_value);
  }

  /**
   * Atomically compare the memory location with the old value, and 
   * if they are equal set the new value, otherwise don't set anything.
   * 
   * Returns the initial value at ptr.
   */
  static inline int atomic_compare_and_swap_val(int* ptr, int old_value, int new_value) {
    return __sync_val_compare_and_swap(ptr, old_value, new_value);
  }
  
  
# ifdef __GNUC__
  static inline uint32_t leading_zeros(uint32_t x)    { return __builtin_clz(x);      }
  static inline uint32_t population_count(uint32_t x) { return __builtin_popcount(x); }
# else
  # warning check whether your compiler provides the following functions as intrinsics 
  uint32_t leading_zeros(uint32_t x) {
    for (int i = 0;  i < 32;  ++i)
      if ( x  &   (1 << (31-i)))  return i;
    return 32;
  }
  
  uint32_t population_count(uint32_t x)  {
    int sum = 0;
    for (int i = 0;  i < 32;  ++i)
      if (x  &  (1 << i))  ++sum;
    return sum;
  }
# endif
  
  
  static inline void* rvm_malloc_shared(size_t sz);
  static inline void* rvm_calloc_shared(size_t num_members, size_t mem_size);
  static inline void  rvm_free_shared(void* mem);
  
  static inline void  mem_fence() { __sync_synchronize(); /*This is a GCC build-in might need to be replaced */ }
  
private:
  static inline void* memalign(int align, int sz) { return (void*) ( (int(malloc(sz + align)) + align - 1) & ~(align-1) ); }
  
  static Interprocess_Allocator* shared_memory_allocator();
  friend void print_interprocess_allocator_heap(); /* Open up for the debugging tools */
  
public:
  static inline void* rvm_memalign(int al, int sz) {
    return memalign(al, sz);
  }
  
  static inline void* rvm_memalign_shared(OS_Heap, int align, int sz) {
    return (void*) ( (int(rvm_malloc_shared(sz + align)) + align - 1) & ~(align-1) );
  }
  
  static inline void* malloc_uncacheable_shared(int alignment, int size) {
    OS_Heap heap;
    return rvm_memalign_shared(heap, alignment, size);
  }
  
  static inline int   mem_create_heap_if_on_Tilera(OS_Heap* heap, bool /* replicate */) {
    heap = NULL; /* unused on POSIX */
    return 0;
  }
  
  static void start_threads  (void (*helper_core_main)(), char* /* argv */[]);
  static void start_processes(void (*helper_core_main)(), char* argv[]);
  
  static inline int get_thread_rank()  { return (int)pthread_getspecific(rank_key); }
  static inline int get_process_rank();
  
  static int abort_if_error(const char*, int); 
  
  static inline void yield_or_spin_a_bit() { assert_always(/*Memory_Semantics::is_using_threads()*/ !On_Tilera); pthread_yield_np(); }

  static void pin_thread_to_core(int32_t rank);

private:
  static void* pthread_thread_main(void* param);
  static int32_t       last_rank;  // needs to be accessed atomically (__sync_fetch_and_add)
  static pthread_key_t rank_key;
  
  static pthread_t     threads  [Max_Number_Of_Cores];
  static pid_t         processes[Max_Number_Of_Cores];
  
  static void create_threads(const size_t num_of_threads, void (*helper_core_main)());

  
public:
  
  static bool ask_for_huge_pages(int desired_huge_pages) {
    if ((On_Apple | On_Intel_Linux) || desired_huge_pages == 0)
      return true;
    return false;
  }

};

# endif // !On_Tilera
