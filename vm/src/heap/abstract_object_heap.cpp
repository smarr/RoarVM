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



void Abstract_Object_Heap::initialize() {
  int size_in_oops = heap_byte_size() / sizeof(Oop);
  int size_in_bytes = size_in_oops * sizeof(Oop);
  initialize(allocate_my_space(size_in_bytes), size_in_bytes);
 }

void Abstract_Object_Heap::initialize(void* mem, int size) {
  _start = _next = (Oop*)mem;
  _end = _next + size/sizeof(Oop);
  zap_unused_portion();
  lowSpaceThreshold = 1000;
}


// ObjectMemory object enumeration

Object* Abstract_Object_Heap::firstAccessibleObject() {
  FOR_EACH_OBJECT_IN_HEAP(this, obj)
    if (!obj->isFreeObject())  return obj;
  return NULL;
}


Oop Abstract_Object_Heap::initialInstanceOf(Oop classPointer) {
  // "Support for instance enumeration. Return the first instance of the given class, or nilObj if it has no instances."
  FOR_EACH_OBJECT_IN_HEAP(this, obj)
    if (!obj->isFreeObject()  &&  obj->fetchClass() == classPointer )
      return obj->as_oop();
  return The_Squeak_Interpreter()->roots.nilObj;
}


Object* Abstract_Object_Heap::first_object_or_null() {
  if (_start == _next)
    return NULL;
  return object_from_chunk((Chunk*)startOfMemory());
}

Object* Abstract_Object_Heap::first_object_without_preheader() {
  if (_start == _next)
    return NULL;
  return object_from_chunk_without_preheader((Chunk*)startOfMemory());
}


bool Abstract_Object_Heap::verify() {
  if (!is_initialized()) return true;

  bool ok = true;
  Object *prev_obj = NULL;
  __attribute__((unused)) Object *prev_prev_obj = NULL; // debugging
  FOR_EACH_OBJECT_IN_HEAP(this, obj) {
    if (obj->is_marked()) {
      lprintf("object 0x%x should not be marked but is; header is 0x%x, in heaps[%d][%d]\n",
      obj, obj->baseHeader, obj->rank(), obj->mutability());
      fatal("");
    }
    if (!obj->isFreeObject() &&  obj->is_current_copy())
      ok = obj->verify() && ok;
    
    if (!ok) dittoing_stdout_printer->printf("Failed to verify obj at %p\n", obj);
    
    prev_prev_obj = prev_obj;
    prev_obj = obj;
  }
  dittoing_stdout_printer->printf("Object_Heap %sverified\n", ok ? "" : "NOT ");
  return ok;
}

void Abstract_Object_Heap::check_multiple_stores_for_generations_only( Oop dsts[], oop_int_t n) {
  return; // unused
  for (oop_int_t i = 0;  i < n;  ++i)
    check_store( &dsts[i] );
}

void Abstract_Object_Heap::multistore( Oop* dst, Oop* end, Oop src) {
  multistore(dst, src, end - dst);
}
void Abstract_Object_Heap::multistore( Oop* dst, Oop src, oop_int_t n) {
  assert(The_Memory_System()->contains(dst));
  oopset_no_store_check(dst, src, n /* never read-mostly */);
  check_multiple_stores_for_generations_only(dst, n);
}
void Abstract_Object_Heap::multistore( Oop* dst, Oop* src, oop_int_t n) {
  assert(The_Memory_System()->contains(dst));

  The_Memory_System()->enforce_coherence_before_store(dst, n << ShiftForWord /* never read-mostly */);
  DEBUG_MULTIMOVE_CHECK(dst, src, n);
  memmove(dst, src, n * bytes_per_oop);
  The_Memory_System()->enforce_coherence_after_store(dst, n << ShiftForWord);
  check_multiple_stores_for_generations_only(dst, n);
}



// verify does it anyway
void Abstract_Object_Heap::ensure_all_unmarked() {
  FOR_EACH_OBJECT_IN_HEAP(this, obj)
    assert_always(!obj->is_marked());
}


void Abstract_Object_Heap::do_all_oops(Oop_Closure* oc) {
  FOR_EACH_OBJECT_IN_HEAP(this, obj)
    obj->do_all_oops_of_object(oc);
}

void Abstract_Object_Heap::scan_compact_or_make_free_objects(bool compacting, Abstract_Mark_Sweep_Collector* gc_or_null) {
  bool for_gc = gc_or_null != NULL;
  // enforce mutability at higher level
  if (for_gc || compacting)
    The_Memory_System()->object_table->pre_store_whole_enchillada();

  if (compacting) ++compactionsSinceLastQuery;

  Chunk* dst_chunk = (Chunk*)startOfMemory();
  for (__attribute__((unused))
       Chunk *src_chunk = dst_chunk,
        *next_src_chunk = NULL,
        *prev_src_chunk = NULL; // prev is only for debugging
       src_chunk  <  (Chunk*)end_objects();
       prev_src_chunk = src_chunk,  src_chunk = next_src_chunk) {


    Object* obj = src_chunk->object_from_chunk();
    next_src_chunk = obj->nextChunk();

    if (obj->isFreeObject())
      continue;

    Oop oop = obj->as_oop();

    if (for_gc) {
      if (!obj->is_marked()) {
        The_Memory_System()->object_table->free_oop(oop  COMMA_FALSE_OR_NOTHING);
        if (!compacting)
          src_chunk->make_free_object((char*)next_src_chunk - (char*)src_chunk, 0);
        continue;
      }
      obj->unmark_without_store_barrier();
    }
    if (!compacting)
      continue;

    Object_p new_obj_addr = (Object_p)(Object*)((char*)dst_chunk + ((char*)obj - (char*)src_chunk));

    if (src_chunk == dst_chunk)
      dst_chunk = next_src_chunk;

    else {
      if (Use_Object_Table)
        The_Memory_System()->object_table->set_object_for(oop, new_obj_addr  COMMA_FALSE_OR_NOTHING); // noop unless indirect oops
      else
        fatal("GC is currently not supported, and this should never happen.");
      
      int n_oops = (Oop*)next_src_chunk - (Oop*)src_chunk;
      // no mutability barrier wanted here, may need generational store barrier in the future
      
      DEBUG_MULTIMOVE_CHECK(dst_chunk, src_chunk, n_oops);
      memmove(dst_chunk, src_chunk, n_oops * sizeof(Oop));
      src_chunk = next_src_chunk;
      dst_chunk = (Chunk*)&((Oop*)dst_chunk)[n_oops];
    }
  }
  if (compacting)
    set_end_objects((Oop*)dst_chunk);


  if (for_gc || compacting)
    The_Memory_System()->object_table->post_store_whole_enchillada();

  // enforce coherence at higher level
}


void Abstract_Object_Heap::zap_unused_portion() {
  assert_always(end_of_space() != NULL);
  if (check_many_assertions) {
    enforce_coherence_before_store(end_objects(), (char*)end_of_space() - (char*)end_objects());

    for (Oop* p = (Oop*)end_objects();
         p < (Oop*)end_of_space();
         *p++ = Oop::from_bits(Oop::Illegals::zapped)) {}

    enforce_coherence_after_store(end_objects(), (char*)end_of_space() - (char*)end_objects());
  }
}



void Abstract_Object_Heap::print(FILE*) {
  lprintf("start 0x%x, next 0x%x, end 0x%x\n", _start, _next, _end);
}

