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


# if On_Apple

#include <sys/sysctl.h>

class OSX_OS_Interface : public POSIX_OS_Interface {
public:
  
  /**
   * Returns true if the current process is being debugged (either 
   * running under the debugger or has a debugger attached post facto).
   * Source: http://developer.apple.com/library/mac/qa/qa1361/
   */
  static bool AmIBeingDebugged(void)
  {
    int                 junk;
    int                 mib[4];
    struct kinfo_proc   info;
    size_t              size;
    
    // Initialize the flags so that, if sysctl fails for some bizarre 
    // reason, we get a predictable result.
    
    info.kp_proc.p_flag = 0;
    
    // Initialize mib, which tells sysctl the info we want, in this case
    // we're looking for information about a specific process ID.
    
    mib[0] = CTL_KERN;
    mib[1] = KERN_PROC;
    mib[2] = KERN_PROC_PID;
    mib[3] = getpid();
    
    // Call sysctl.
    
    size = sizeof(info);
    junk = sysctl(mib, sizeof(mib) / sizeof(*mib), &info, &size, NULL, 0);
    assert(junk == 0);
    
    // We're being debugged if the P_TRACED flag is set.
    
    return ( (info.kp_proc.p_flag & P_TRACED) != 0 );
  }
  
  static inline void breakpoint() { 
    if (AmIBeingDebugged())
      raise(SIGTRAP);
  }
  
  static void ensure_Time_Machine_backs_up_run_directory();

# if On_iOS
  static inline void moncontrol(int) {}
# endif

  static inline void profiler_enable()  { moncontrol(1); }
  static inline void profiler_disable() { moncontrol(0); }
  static inline void profiler_clear()   {}
  
  static Power_Source get_power_source();
  
  static void pin_thread_to_core(int32_t rank);
  
  
# if Use_Spin_Locks
  typedef OSSpinLock Mutex;
  
  static inline void mutex_init(Mutex* mutex, const void* _ = NULL) {
    *mutex = 0;
  }
  
  static inline void mutex_destruct(Mutex* mutex) {}
  
  static inline int mutex_lock(Mutex* mutex) {
    OSSpinLockLock(mutex);
    return 0;
  }
  
  static inline bool mutex_trylock(Mutex* mutex) {
    return OSSpinLockTry(mutex);
  }
  
  static inline int mutex_unlock(Mutex* mutex) {
    OSSpinLockUnlock(mutex);
    return 0;
  }
  
# endif
  
# if On_iOS
# warning STEFAN: we might want to reconsider this hack
  char* map_heap_memory(size_t total_size,
                        size_t /* bytes_to_map */,
                        void*  /* where */,      off_t  /* offset */,
                        int    /* main_pid */,   int    /* flags */) {
    return (char*)malloc(total_size);
  }
# endif

  
};

# endif // On_Apple
