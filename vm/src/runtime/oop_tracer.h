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


class Oop_Tracer: public Abstract_Tracer {
public:
  Oop_Tracer(int n) : Abstract_Tracer(n, sizeof(Oop), 1) {}
  void do_all_roots(Oop_Closure*);
  void add(Oop x) {
    ((Oop*)buffer)[get_free_entry()] = x;
  }

protected:
  Oop array_class();
  void copy_elements(int src_offset, void* dst, int dst_offset, int num_elems, Object_p dst_obj) {
    bcopy((Oop*)buffer + src_offset, (Oop*)dst + dst_offset, num_elems * sizeof(Oop));
    //dst_obj->my_heap()->check_multiple_stores_for_generations_only((Oop*)dst, num_elems); //unused
  }

};

