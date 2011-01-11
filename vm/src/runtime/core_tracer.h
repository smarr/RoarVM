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


class Core_Tracer: public Abstract_Tracer {
 public:
  Core_Tracer(int n) : Abstract_Tracer(n, sizeof(char), 1) {}
  void add(char i) {
    ((char*)buffer)[get_free_entry()] = i;
  }
  void copy_elements(int src_offset, void* dst, int dst_offset, int num_elems, Object_p dst_obj) {
    bcopy((char*)buffer + src_offset, (char*)dst + dst_offset, num_elems * sizeof(char));
  }


 protected:
  Oop array_class();
};

