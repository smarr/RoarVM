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


# if Collect_Performance_Counters
  // use like PERF_CNT(The_Squeak_Interpreter(), add_interpret_cycles(foo - start));
  # define PERF_CNT(interp, counter_or_accumulator_call) interp->perf_counter.counter_or_accumulator_call
  
# else
  # define PERF_CNT(interp, counter_or_accumulator_call)

# endif  // Collect_Performance_Counters



class Performance_Counters {
private:

  static Performance_Counters* _all_perf_counters[Max_Number_Of_Cores];
  
  # define FOR_ALL_PERFORMANCE_COUNTERS_DO(template) \
    template(acquire_safepoint,           int, 0) \
    template(acquire_scheduler_mutex,     int, 0) \
    template(send_intercore_messages,     int, 0) \
    template(received_intercore_messages, int, 0) \
    template(full_gc,                     int, 0) \
    template(methods_executed,            int, 0) \
    template(primitive_invokations,       int, 0) \
    template(bytecodes_executed,          int, 0) \
    \
    /* Stats from within Squeak_Interpreter::multicore_interrupt() */ \
    template(multicore_interrupts,        int, 0) \
    template(multicore_interrupt_check,   int, 0) \
    template(yield_requested,             int, 0) \
    template(data_available,              int, 0) \
 
 
  # define FOR_ALL_PERFORMANCE_ACCUMULATORS_DO(template) \
    template(interpret_cycles,            u_int64, 0LL) \
    template(multicore_interrupt_cycles,  u_int64, 0LL) \
    template(mi_cyc_1,                    u_int64, 0LL) \
    template(mi_cyc_1a,                   u_int64, 0LL) \
    template(mi_cyc_1a1,                  u_int64, 0LL) \
    template(mi_cyc_1a2,                  u_int64, 0LL) \
    template(mi_cyc_1b,                   u_int64, 0LL) \


  # define DECLARE_MEMBERS(name, type, initial_value) \
    type name;
  
  FOR_ALL_PERFORMANCE_COUNTERS_DO(DECLARE_MEMBERS)

  FOR_ALL_PERFORMANCE_ACCUMULATORS_DO(DECLARE_MEMBERS)
  
  # undef DECLARE_MEMBERS

  
public:
  
  Performance_Counters();
  
# if Collect_Performance_Counters
  
  # define DECLARE_COUNTER_METHODS(name, type, initial_value) \
    FORCE_INLINE void count_##name() { \
      /* Not necessary anymore OS_Interface::atomic_fetch_and_add(&name, 1); */ \
      name += 1; \
    } \
    \
    static void count_##name##_static();

  #define DECLARE_ACCUMULATOR_METHODS(name, type, initial_value) \
    FORCE_INLINE void add_##name(type value) {\
      name += value; \
    } \
    \
    static void add_##name##_static(type value);
  
# else

  # define DECLARE_COUNTER_METHODS(name, type, initial_value) \
    FORCE_INLINE void count_##name() const {} \
    \
    static FORCE_INLINE void count_##name##_static() {};
  
  # define DECLARE_ACCUMULATOR_METHODS(name, type, initial_value) \
    FORCE_INLINE void add_##name(type) const {} \
    \
    static void add_##name##_static(type) {}

# endif  

  
  FOR_ALL_PERFORMANCE_COUNTERS_DO    (DECLARE_COUNTER_METHODS)
  FOR_ALL_PERFORMANCE_ACCUMULATORS_DO(DECLARE_ACCUMULATOR_METHODS)
  
  
  # undef DECLARE_COUNTER_METHODS
  # undef DECLARE_ACCUMULATOR_METHODS
  
  
  # define DECLARE_GETTERS(name, type, initial_value) \
    type get_##name() const {\
      return name; \
    }
  
  FOR_ALL_PERFORMANCE_COUNTERS_DO    (DECLARE_GETTERS)
  FOR_ALL_PERFORMANCE_ACCUMULATORS_DO(DECLARE_GETTERS)

  # undef DECLARE_GETTERS
  

  
  static void print();
  static void reset();
  
  void reset_accumulators() {
    # define INITIALIZE(name, type, initial_value) \
      name = initial_value;
    
    FOR_ALL_PERFORMANCE_ACCUMULATORS_DO(INITIALIZE)
    
    # undef INITIALIZE
  }
  
};
