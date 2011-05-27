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


# include "headers.h"


Performance_Counters* Performance_Counters::_all_perf_counters[Max_Number_Of_Cores] = { NULL };

Performance_Counters::Performance_Counters() {
  // First, lets register this performce counter in the static array
  // this will allow us to easily get all instances for printing, etc.
  _all_perf_counters[Logical_Core::my_rank()] = this;
  
  // Now initialize everything with its initial value
  # define INITIALIZE(name, type, initial_value) \
    name = initial_value;
  
  FOR_ALL_PERFORMANCE_COUNTERS_DO(INITIALIZE)
  FOR_ALL_PERFORMANCE_ACCUMULATORS_DO(INITIALIZE)
  
  # undef INITIALIZE
}

# if Collect_Performance_Counters

# define IMPL_STATIC_COUNTER_METHODS(name, type, initial_value) \
  void Performance_Counters::count_##name##_static() {\
    The_Squeak_Interpreter()->perf_counter.count_##name(); \
  }

# define IMPL_STATIC_ACCUMULATOR_METHODS(name, type, initial_value) \
  void Performance_Counters::add_##name##_static(type value) {\
    The_Squeak_Interpreter()->perf_counter.add_##name(value); \
  }


FOR_ALL_PERFORMANCE_COUNTERS_DO(IMPL_STATIC_COUNTER_METHODS)

FOR_ALL_PERFORMANCE_ACCUMULATORS_DO(IMPL_STATIC_ACCUMULATOR_METHODS)

# undef IMPL_STATIC_COUNTER_METHODS
# undef IMPL_STATIC_ACCUMULATOR_METHODS


# endif // Collect_Performance_Counters


void Performance_Counters::print() {
  if (!Collect_Performance_Counters)
    return;
  
  fprintf(stdout, "Performance Counters:\n");
  
  FOR_ALL_RANKS(r) {
    fprintf(stdout, "\tRank %d:\n", r);
    
    # define PRINT(name, type, initial_value) fprintf(stdout, "\t %-30s = %8d\n", #name, _all_perf_counters[r]->name);

    FOR_ALL_PERFORMANCE_COUNTERS_DO(PRINT)
    
    # undef PRINT

    
    # define PRINT(name, type, initial_value) fprintf(stdout, "\t %-30s = %8lld\n", #name, _all_perf_counters[r]->name);
    
    FOR_ALL_PERFORMANCE_ACCUMULATORS_DO(PRINT)
    
    # undef PRINT

    fprintf(stdout, "\n");
    
  }

}

void Performance_Counters::reset() {
  if (!Collect_Performance_Counters)
    return;
  
  FOR_ALL_RANKS(r) {
  
    # define RESET_ALL_COUNTERS(name, type, initial_value) \
      _all_perf_counters[r]->name = initial_value;
  
    FOR_ALL_PERFORMANCE_COUNTERS_DO(RESET_ALL_COUNTERS)
  
    # undef RESET_ALL_COUNTERS
  
    OS_Interface::mem_fence(); // STEFAN: I think, I actually want a flush here, but do not see it in the GCC manual
  }
}

