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


class Debugging_Tracer : public Abstract_Tracer {
  bool is_during_remote_context_allocation;
  int remote_context_allocation_requests;
  int remote_context_allocations;
  int gc_count;
 public:
  Debugging_Tracer() : Abstract_Tracer(1, 1, 1) {
    is_during_remote_context_allocation = false;
    remote_context_allocation_requests = 0;
    remote_context_allocations = 0;
  }
  void do_all_roots(Oop_Closure*) {}
  void starting_remote_context_allocation() {
    ++remote_context_allocation_requests;
    is_during_remote_context_allocation = true;
  }
  void finishing_remote_context_allocation() {
    is_during_remote_context_allocation = false;
  }
  bool force_gc();
  bool force_real_context_allocation();
  void record_gc();

protected:
  Oop array_class() { fatal("inappropriate"); return Oop::from_bits(0); }
  void copy_elements(int src_offset, void* dst, int dst_offset, int num_elems, Object* dst_obj) {
    fatal("inappropriate");
  }
};

