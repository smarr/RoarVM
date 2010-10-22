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


class Process_Field_Locator {
  public:  
  Process_Field_Locator () {
    names[suspendedContext] = "suspendedContext";
    names[hostCore] = "hostCore";
    names[coreMask] = "coreMask";
    last_timestamp = -1;
  }

  enum Fields {
    suspendedContext, // don't really need this one, but it allows assertion checking -- dmu 7/10
    hostCore,
    coreMask,
    count
  };
  private:
  
  oop_int_t indices[count];
  const char* names[count];
  int32 last_timestamp;
  
  Object* instance_variable_names_of_Process();  
  int instance_variable_count_of_superclasses_of_Process();
  void update_indices();
  static Object* class_process();
  
  public:
  int index_of_process_inst_var(Fields);
  bool processes_have_RVM_fields();
  void print_results();
};

extern Process_Field_Locator The_Process_Field_Locator;
