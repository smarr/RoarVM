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

class OSX_OS_Interface : public POSIX_OS_Interface {
public:

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
  
  static inline int mutex_trylock(Mutex* mutex) {
    return OSSpinLockTry(mutex);
  }
  
  static inline int mutex_unlock(Mutex* mutex) {
    OSSpinLockUnlock(mutex);
    return 0;
  }
  
# endif
  
};

# endif // On_Apple
