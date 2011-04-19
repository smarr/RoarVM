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


class External_Primitive_Table: public Abstract_Primitive_Table {
 public:


  void* operator new(size_t size) {
    return Memory_Semantics::shared_malloc(size);
  }

  External_Primitive_Table() : Abstract_Primitive_Table(4096, true) { }

   int add(fn_t addr, bool on_main) {
    for (int i = 0;  i < size;  ++i)
      if (contents[i] == NULL) {
        contents[i] = addr;
        execute_on_main[i] = on_main;
        return i + 1;
      }
    lprintf("External_Primitive_Table is too small!");
    return lookup_failed;
  }

};

