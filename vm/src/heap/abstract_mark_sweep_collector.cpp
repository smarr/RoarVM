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


bool Abstract_Mark_Sweep_Collector::print_gc = true;


Abstract_Mark_Sweep_Collector::Abstract_Mark_Sweep_Collector() {
  mark_stack = NULL;
  weakRootCount = 0;
  weakRoot_accessor = NULL;
}


void Abstract_Mark_Sweep_Collector::gc() {
  int rank_on_threads_or_zero_on_processes = Memory_Semantics::rank_on_threads_or_zero_on_processes();

  if (Trace_Execution  &&  The_Squeak_Interpreter()->execution_tracer() != NULL)
    The_Squeak_Interpreter()->execution_tracer()->record_gc();
  if (Trace_For_Debugging  &&  The_Squeak_Interpreter()->debugging_tracer() != NULL)
    The_Squeak_Interpreter()->debugging_tracer()->record_gc();

  Safepoint_for_moving_objects sf("gc");
  Safepoint_Ability sa(false);
  
  // Relies on implicit init to false below:
  static cacheline_aligned<bool> recursing[Memory_Semantics::max_num_threads_on_threads_or_1_on_processes]; // threadsafe, GC are started concurrently on multiple cores as far as I can see, Stefan 2009-09-05

  if (recursing[rank_on_threads_or_zero_on_processes].value == true) fatal("recursing in fullGC");
  recursing[rank_on_threads_or_zero_on_processes].value = true;

  assert(The_Squeak_Interpreter()->safepoint_tracker->have_acquired_safepoint());
  flushFreeContextsMessage_class().send_to_all_cores();
  prepare(true);
  do_it();
  finish();

  recursing[rank_on_threads_or_zero_on_processes].value = false;
}

void Abstract_Mark_Sweep_Collector::mark_only_for_debugging() {
  prepare(false);
  mark();
}


void Abstract_Mark_Sweep_Collector::prepare(bool verify) {
  if (print_gc)
    dittoing_stdout_printer->printf("starting gc on %d\n", Logical_Core::my_rank());

  mark_stack = new GC_Oop_Stack();
  The_Memory_System()->verify_if(check_many_assertions && verify);
  The_Squeak_Interpreter()->preGCAction_everywhere(true);
}


void Abstract_Mark_Sweep_Collector::do_it() {
  if (print_gc)
    lprintf("finished preparing; about to mark\n");
  mark();
  if (check_assertions)
    The_Memory_System()->object_table->verify_after_mark();
  if (print_gc)
    lprintf("finished marking, starting sweeping\n");
  finalize_weak_arrays();
  sweep_unmark_and_compact_or_free(this);
}


void Abstract_Mark_Sweep_Collector::finalize_weak_arrays() {
  for (u_int32 i = 0;  i < weakRootCount; ++i)
    finalizeReference(weakRoots[i].as_object());
  weakRootCount = 0;
}



void Abstract_Mark_Sweep_Collector::finish() {
  The_Squeak_Interpreter()->postGCAction_everywhere(true);

  The_Memory_System()->verify_if(check_many_assertions);

  if (print_gc)
    dittoing_stdout_printer->printf("finishing gc\n");
}



class Mark_Closure: public Oop_Closure {
  Abstract_Mark_Sweep_Collector* gc;
public:
  Mark_Closure(Abstract_Mark_Sweep_Collector* x) : Oop_Closure() {gc = x;}
  void value(Oop* p, Object_p) { gc->mark(p); }
  virtual const char* class_name(char*) { return "Mark_Closure"; }
};


void Abstract_Mark_Sweep_Collector::mark() {
  The_Memory_System()->enforce_coherence_before_this_core_stores_into_all_heaps();
  Mark_Closure mc(this);
  The_Interactions.do_all_roots_here(&mc);

  while (!mark_stack->is_empty())
    mark_an_object(mark_stack->pop());

  The_Memory_System()->enforce_coherence_after_this_core_has_stored_into_all_heaps();

  if (!mark_stack->is_empty()) fatal("");
  delete mark_stack;
  mark_stack = NULL;

}



void Abstract_Mark_Sweep_Collector::sweep_unmark_and_compact_or_free(Abstract_Mark_Sweep_Collector* gc_or_null) {
  unmark_maybe_compact_set_translation_buffer_if_no_OT(gc_or_null);
}

void Abstract_Mark_Sweep_Collector::unmark_maybe_compact_set_translation_buffer_if_no_OT(Abstract_Mark_Sweep_Collector* gc_or_null) {
  The_Memory_System()->scan_compact_or_make_free_objects_everywhere(true, gc_or_null);
}


bool Abstract_Mark_Sweep_Collector::add_weakRoot(Oop x) {
  if (weakRoot_accessor == NULL)  weakRoot_accessor = Logical_Core::my_core();
  else if (weakRoot_accessor != Logical_Core::my_core()) fatal("must be accessed from same core");

  if (weakRootCount >= sizeof(weakRoots) / sizeof(weakRoots[0]))
    return false;

  weakRoots[weakRootCount++] = x;
  return true;
}





/*
 xxxxxx_weak

 finalizeReference: oop
 "During sweep phase we have encountered a weak reference.
 Check if  its object has gone away (or is about to) and if so, signal a
 semaphore. "
 "Do *not* inline this in sweepPhase - it is quite an unlikely
 case to run into a weak reference"
 | weakOop oopGone chunk firstField lastField |
 self inline: false.
 firstField := BaseHeaderSize + ((self nonWeakFieldsOf: oop) << ShiftForWord).
 lastField := self lastPointerOf: oop.
 firstField to: lastField by: BytesPerWord do: [:i |
 weakOop := self longAt: oop + i.
 "ar 1/18/2005: Added oop < youngStart test to make sure we're not testing
 objects in non-GCable region. This could lead to a forward reference in
 old space with the oop pointed to not being marked and thus treated as free."
 (weakOop == nilObj or: [(self isIntegerObject: weakOop) or:[weakOop < youngStart]])

 ifFalse: ["Check if the object is being collected.
 If the weak reference points
 * backward: check if the weakOops chunk is free
 * forward: check if the weakOoop has been marked by GC"
 weakOop < oop
 ifTrue: [
 chunk := self chunkFromOop: weakOop.
 oopGone := ((self longAt: chunk) bitAnd: TypeMask) = HeaderTypeFree]
 ifFalse: [oopGone := ((self baseHeader: weakOop) bitAnd: MarkBit) = 0].
 oopGone ifTrue: ["Store nil in the pointer and signal the  interpreter "
 self longAt: oop + i put: nilObj.
 self signalFinalization: oop]]]


 */

void Abstract_Mark_Sweep_Collector::finalizeReference(Object_p weak_obj) {
  int nonWeakCnt = weak_obj->nonWeakFieldsOf();
  FOR_EACH_WEAK_OOP_IN_OBJECT(weak_obj, oop_ptr) {
    Oop x = *oop_ptr;
    if (    x.is_mem()
        &&  x != The_Squeak_Interpreter()->roots.nilObj
        &&  has_been_or_will_be_freed_by_this_ongoing_gc(x)) {
      *oop_ptr = The_Squeak_Interpreter()->roots.nilObj; // no store checks, no coherence operations, in the midst of GC
      if (nonWeakCnt >= 2) weak_obj->weakFinalizerCheckOf();
      The_Squeak_Interpreter()->signalFinalization(x);
    }
  }
}


bool Abstract_Mark_Sweep_Collector::has_been_or_will_be_freed_by_this_ongoing_gc(Oop x) {
  return x.is_mem()
     &&  x != The_Squeak_Interpreter()->roots.nilObj
     &&  ( The_Memory_System()->object_table->is_OTE_free(x) ||  !x.as_object()->is_marked());
}

