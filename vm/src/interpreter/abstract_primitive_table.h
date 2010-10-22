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


class Abstract_Primitive_Table {
public:
  fn_t *contents;
  bool *execute_on_main;

  // special 1-origin index values for external prims
  static const oop_int_t lookup_needed = 0;
  static const oop_int_t lookup_failed = -1;
  static bool  is_index_valid(oop_int_t index) { return index > 0; }


  Abstract_Primitive_Table(int s, bool must_be_shared) {
    size = s;
    contents        = must_be_shared ? (fn_t*)Memory_Semantics::shared_malloc(s * sizeof(fn_t)) : new fn_t[s];
    execute_on_main = must_be_shared ? (bool*)Memory_Semantics::shared_malloc(s * sizeof(bool)) : new bool[s];
    flush();
  }
  int size;
  void flush() {
    for (int i = 0;  i < size;  ++i)
      contents[i] = 0;
  }
};

