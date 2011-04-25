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
<<<<<<< HEAD
 public:
=======
public:
>>>>>>> f4a658a7aeee0ea2d5d8612d75dfb6f910865e62


  void* operator new(size_t size) {
    return Memory_Semantics::shared_malloc(size);
  }

  External_Primitive_Table() : Abstract_Primitive_Table(4096, true) { }

<<<<<<< HEAD
   int add(fn_t addr, bool on_main) {
=======
  int add(fn_t addr, bool on_main) {
>>>>>>> f4a658a7aeee0ea2d5d8612d75dfb6f910865e62
    for (int i = 0;  i < size;  ++i)
      if (contents[i] == NULL) {
        // Entry is empty, lets try to set it
        if (OS_Interface::atomic_compare_and_swap((int*)&contents[i], NULL, (int)addr)) {
<<<<<<< HEAD
        execute_on_main[i] = on_main;
        return i + 1;
=======
          execute_on_main[i] = on_main;
          return i + 1;
>>>>>>> f4a658a7aeee0ea2d5d8612d75dfb6f910865e62
        }
        else {
          // Setting new address failed because of a concurrent modification
          assert(contents[i] != NULL);
        }
      }
    lprintf("External_Primitive_Table is too small!");
    return lookup_failed;
  }

};

