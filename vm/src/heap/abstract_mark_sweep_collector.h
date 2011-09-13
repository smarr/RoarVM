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


class Abstract_Mark_Sweep_Collector {
public:
  static bool print_gc; // threadsafe: set-once config flag

  Abstract_Mark_Sweep_Collector();

  void gc();

  void mark_only_for_debugging();


protected:
  virtual void prepare(bool);
  virtual void do_it();
  virtual void finish();

  void mark();
  GC_Oop_Stack* mark_stack;

  u_int32   weakRootCount;
  Oop       weakRoots[10000];
  Logical_Core* weakRoot_accessor;
 public:
  bool add_weakRoot(Oop);

 public:

  void mark(Oop* p) {
    if (!p) return;
    if (!p->is_mem()) return;
    
    Object* o = p->as_untracked_object_ptr();
    if (o->isFreeObject())
      fatal();
    if (o->is_marked())
      return;
    mark_stack->push(o);
    o->mark_without_store_barrier();
  }

  void mark_an_object(Object* o) {
    // was o->do_all_oops_of_object(&mark_closure);
    o->do_all_oops_of_object_for_marking(this);
  }
 protected:
  void sweep_unmark_and_compact_or_free(Abstract_Mark_Sweep_Collector*);
  void unmark_maybe_compact_set_translation_buffer_if_no_OT(Abstract_Mark_Sweep_Collector*);


 public:
  void finalizeReference(Object_p);
 protected:
  bool has_been_or_will_be_freed_by_this_ongoing_gc(Oop x);
  void finalize_weak_arrays();
 };

