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


class Performance_Counters {
private:
  
  # define FOR_ALL_PERFORMANCE_COUNTERS_DO(template) \
    template(acquire_safepoint,           int, 0) \
    template(acquire_scheduler_mutex,     int, 0) \
    template(send_intercore_messages,     int, 0) \
    template(received_intercore_messages, int, 0) \
    template(full_gc,                     int, 0) \
    template(methods_executed,            int, 0) \
    template(primitive_invokations,       int, 0) \
    template(bytecodes_executed,          int, 0) \
    template(multicore_interrupts,        int, 0) \
  
  
  # define DECLARE_COUNTER_MEMBERS(name, type, initial_value) \
      static type name;

  FOR_ALL_PERFORMANCE_COUNTERS_DO(DECLARE_COUNTER_MEMBERS)
  
  # undef DECLARE_COUNTER_MEMBERS

  
public:
  
# if Collect_Performance_Counters
  
  # define DECLARE_COUNTER_METHODS(name, type, initial_value) \
      static FORCE_INLINE void count_##name() { \
        OS_Interface::atomic_fetch_and_add(&name, 1); \
      }

# else

  # define DECLARE_COUNTER_METHODS(name, type, initial_value) \
      static FORCE_INLINE void count_##name() {}

  
# endif  
  
  FOR_ALL_PERFORMANCE_COUNTERS_DO(DECLARE_COUNTER_METHODS)
  
  # undef DECLARE_COUNTER_METHODS
  
  
  static void print();

  static void reset() {
    # define RESET_ALL_COUNTERS(name, type, initial_value) \
      name = initial_value;
    
    FOR_ALL_PERFORMANCE_COUNTERS_DO(RESET_ALL_COUNTERS)
    
    # undef RESET_ALL_COUNTERS
    
    OS_Interface::mem_fence(); // STEFAN: I think, I actually want a flush here, but do not see it in the GCC manual
    
    print();
  };
  
};
