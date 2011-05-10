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


# define DECLARE_COUNTER_INITIALIZERS(name, type, initial_value) \
  type Performance_Counters::name = initial_value;

FOR_ALL_PERFORMANCE_COUNTERS_DO(DECLARE_COUNTER_INITIALIZERS)

# undef DECLARE_COUNTER_INITIALIZERS


void Performance_Counters::print() {
  if (!Collect_Performance_Counters)
    return;
  
  fprintf(stdout, "Performance Counters:\n");
  # define PRINT(name, type, initial_value) fprintf(stdout, " %-30s = %8d\n", #name, name);
  FOR_ALL_PERFORMANCE_COUNTERS_DO(PRINT)
  fprintf(stdout, "\n");
# undef PRINT
}