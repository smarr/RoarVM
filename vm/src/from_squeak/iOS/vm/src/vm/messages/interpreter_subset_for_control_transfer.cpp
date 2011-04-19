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


#include "headers.h"

void Interpreter_Subset_For_Control_Transfer::set_from_interpreter() {
  // other side will be interpreting so be sure all objects, e.g. active contexts are current to it
  The_Squeak_Interpreter()->preGCAction_here(false); // in case GC happens during the prim, need to be able to retrieve values of IP and SP later, if this call is for the interp on main core
  if (The_Squeak_Interpreter()->fence())
    OS_Interface::mem_fence();

# define SET_FROM(type, my_var, in_var) my_var = The_Squeak_Interpreter()->in_var;
  FOR_ALL_VARS_IN_SUBSET(SET_FROM)
# undef SET_FROM
}


void Interpreter_Subset_For_Control_Transfer::fill_in_interpreter() {
  Safepoint_Ability sa(false); // no hanky-panky while we fill
  
# define FILL_FROM(type, my_var, in_var) The_Squeak_Interpreter()->in_var = my_var;
  FOR_ALL_VARS_IN_SUBSET(FILL_FROM)
# undef FILL_FROM
  The_Squeak_Interpreter()->postGCAction_here(false); // resync Object*'s and ip and sp with Oops in case GC happened on main while doing the primitive

  if (Track_Processes)
    The_Squeak_Interpreter()->running_process_by_core[Logical_Core::my_rank()] = running_process_or_nil;

  if (Print_Scheduler) {
    debug_printer->printf("scheduler: on %d receive_for_control_transfer set_running_process: ", Logical_Core::my_rank());
    running_process_or_nil.as_object()->print_process_or_nil(debug_printer);
    debug_printer->nl();
  }
  The_Squeak_Interpreter()->assert_stored_if_no_proc();
  if (Trace_Execution && The_Squeak_Interpreter()->execution_tracer() != NULL)
    The_Squeak_Interpreter()->execution_tracer()->received_current_bytecode();
  assert_eq(The_Squeak_Interpreter()->activeContext_obj(), (void*)The_Squeak_Interpreter()->activeContext().as_object(), "activeContext");
}


void Interpreter_Subset_For_Control_Transfer::do_all_roots(Oop_Closure* oc) {
# define INVOKE_CLOSURE(type, my_var, in_var) oc->value(&my_var, (Object_p)NULL);
  FOR_ALL_OOPS_IN_SUBSET(INVOKE_CLOSURE)
# undef INVOKE_CLOSURE
}

