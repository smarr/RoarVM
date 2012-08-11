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

/**
 * This file defines helper functions to facilitate debugging.
 * Helper like pst() can be used from GDB with 'call pst()' easily.
 */

// OOP

void dp(Oop x) { x.dp(); }
void Oop::dp() { print(stderr_printer); stderr_printer->nl(); }  // debugging print routines
void dp(Object* x) {x->dp(); } // print Object

// Interpreter

void pet() {
  if (Trace_Execution && The_Squeak_Interpreter()->execution_tracer() != NULL)
    The_Squeak_Interpreter()->execution_tracer()->print();
}

void pst() {
  The_Squeak_Interpreter()->print_stack_trace(dittoing_stdout_printer);
}

int printCallStack() { pst(); return 0; }

void pat() {
  The_Squeak_Interpreter()->print_all_stack_traces(dittoing_stdout_printer);
}
int printAllStacks() { pat(); return 0; }
void disable_context_switches() {
  The_Squeak_Interpreter()->suppress_context_switch_for_debugging = true;
}
void reenable_context_switches() {
  The_Squeak_Interpreter()->suppress_context_switch_for_debugging = false;
}


