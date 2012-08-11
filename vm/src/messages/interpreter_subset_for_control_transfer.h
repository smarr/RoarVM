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


class Interpreter_Subset_For_Control_Transfer {

# if check_assertions
  # define FOR_ALL_ASSERTION_VARS_IN_SUBSET(template) \
    template(bool, are_registers_stored, are_registers_stored) \
    template(bool, _is_internal_valid, _is_internal_valid) \
    template(bool, _is_external_valid, _is_external_valid)
# else
  # define FOR_ALL_ASSERTION_VARS_IN_SUBSET(template)
# endif

# define FOR_ALL_OOPS_IN_SUBSET(template) \
template(Oop, receiver, roots.receiver) \
template(Oop, messageSelector, roots.messageSelector) \
template(Oop, newMethod, roots.newMethod) \
template(Oop, lkupClass, roots.lkupClass) \
template(Oop, _activeContext, roots._activeContext) \
template(Oop, _method, roots._method) \
template(Oop, _theHomeContext, roots._theHomeContext) \
template(Oop, running_process_or_nil, roots.running_process_or_nil) \


# define FOR_ALL_VARS_IN_SUBSET(template) \
template(Object_p, activeContext_obj, _activeContext_obj) \
template(Object_p, method_obj, _method_obj) \
template(Object_p, theHomeContext_obj, _theHomeContext_obj) \
template(u_char*, instructionPointer, _instructionPointer) \
template(Oop*, stackPointer, _stackPointer) \
template(u_char*, _localIP, _localIP) \
template(Oop*, _localSP, _localSP) \
template(u_char, currentBytecode, currentBytecode) \
template(bool, have_executed_currentBytecode, have_executed_currentBytecode) \
template(int, interruptCheckCounter, interruptCheckCounter) \
template(int, reclaimableContextCount, reclaimableContextCount) \
template(bool, successFlag, successFlag) /* for prims */ \
\
FOR_ALL_OOPS_IN_SUBSET(template) \
\
FOR_ALL_ASSERTION_VARS_IN_SUBSET(template)

# define DCL_VAR(type, my_var, in_var) type my_var;

  FOR_ALL_VARS_IN_SUBSET(DCL_VAR)

# undef DCL_VAR

  public:
  void set_from_interpreter();
  void fill_in_interpreter();
  void do_all_roots(Oop_Closure*);
};

