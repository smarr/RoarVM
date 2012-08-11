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


void dp(Oop x); // print Oop
void dp(int x); // print Oop
void dp(Object* x); // print Object

Oop  at(Oop x, oop_int_t i); // get value at index i in object x

void dpf(Oop x);
void dpf(Object* x);

void dpf_top(); // print fields of the object on the top of the stack
void dpf_n(int offset); // prints fields of the object at the offset in the stack

void pat(); // print all stack traces
void pet(); // print execution trace
void pst(); // print stack trace

void print_current_method();
void print_stack_frame();

void disable_context_switches();
void reenable_context_switches();

extern "C" {
  int printCallStack();
  int printAllStacks();
}
