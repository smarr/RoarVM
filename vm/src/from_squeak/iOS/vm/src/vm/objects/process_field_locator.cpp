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

Process_Field_Locator The_Process_Field_Locator;

Object_p Process_Field_Locator::instance_variable_names_of_Process() {
  return class_process()->fetchPointer(class_process()->instance_variable_names_index_of_class("suspendedContext")).as_object();
}

int Process_Field_Locator::instance_variable_count_of_superclasses_of_Process() {
  return Object::ClassFormat::fixedFields( class_process()->superclass().as_object()->formatOfClass() );
}

void Process_Field_Locator::update_indices() {
  if (The_Squeak_Interpreter()->process_object_layout_timestamp() == last_timestamp)  return;
  Object_p instance_variable_names = instance_variable_names_of_Process();
  int n = instance_variable_count_of_superclasses_of_Process();
  for (int i = 0; i < count;  ++i) {
    int index =  instance_variable_names->index_of_string_in_array(names[i]);
   indices[i] = index < 0  ?  index  :  index + n; // if -1 must keep -1 for none
  }
  last_timestamp = The_Squeak_Interpreter()->process_object_layout_timestamp();
  if (check_assertions) print_results();
  if (Object_Indices::SuspendedContextIndex != indices[suspendedContext]) {
    print_results();
    lprintf("Object_Indices::SuspendedContextIndex = %d\n", Object_Indices::SuspendedContextIndex);
    fatal("Process_Field_Locator inconsistent with Object_Indices");
  }
}

int Process_Field_Locator::index_of_process_inst_var(Fields f) { // or -1
  update_indices();
  return indices[f];
}


void Process_Field_Locator::print_results() {
  for (int i = 0; i < count; i++) 
    lprintf("indices[%s] = %d\n", names[i], indices[i]);
}


Object_p Process_Field_Locator::class_process() { return The_Squeak_Interpreter()->splObj_obj(Special_Indices::ClassProcess); }
