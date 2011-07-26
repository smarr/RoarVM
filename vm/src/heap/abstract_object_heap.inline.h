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


inline bool Abstract_Object_Heap::is_read_mostly() { return The_Memory_System()->is_address_read_mostly(startOfMemory()); }

inline bool Abstract_Object_Heap::is_read_write() { return !is_read_mostly(); }

inline void Abstract_Object_Heap:: enforce_coherence_before_store(void* p, int nbytes) {
  assert(The_Memory_System()->contains(p));
  if (is_read_mostly()) The_Memory_System()->pre_cohere(p, nbytes);
}

inline void Abstract_Object_Heap::enforce_coherence_after_store(void* p, int nbytes) {
  if (is_read_mostly()) The_Memory_System()->post_cohere(p, nbytes);
}


inline void Abstract_Object_Heap::enforce_coherence_before_store_into_object_by_interpreter(void* p, int nbytes, Object_p dst_obj_to_be_evacuated) {
  assert(The_Memory_System()->contains(p));
  The_Memory_System()->enforce_coherence_before_store_into_object_by_interpreter(p, nbytes, dst_obj_to_be_evacuated);
}

inline void Abstract_Object_Heap::enforce_coherence_after_store_into_object_by_interpreter(void* p, int nbytes) {
  if (is_read_mostly()) The_Memory_System()->post_cohere(p, nbytes);
}



inline bool Abstract_Object_Heap::sufficientSpaceToAllocate(oop_int_t bytes) {
  u_oop_int_t minFree = lowSpaceThreshold + bytes + Object::BaseHeaderSize;

  if (Trace_GC_For_Debugging && The_Squeak_Interpreter()->debugging_tracer() != NULL  &&  The_Squeak_Interpreter()->debugging_tracer()->force_gc())
    ;
  else if (bytesLeft(false) >= minFree)
    return true;

  if (The_Squeak_Interpreter()->safepoint_ability->is_able()) // might be allocating a context
    The_Memory_System()->fullGC("sufficientSpaceToAllocate");

  if (bytesLeft(false) >= minFree)
    return true;

  /*  implement this
   The_Memory_System()->balanceHeaps();


   if (bytesLeft(false) >= minFree)
   return true;
   */


  return false;
}


inline Object* Abstract_Object_Heap::accessibleObjectAfter(Object* obj) {
  for (;;) {
    obj = next_object(obj);
    if (obj == NULL  ||  !obj->isFreeObject())
      return obj;
  }
}


inline int Abstract_Object_Heap::rank() { return The_Memory_System()->rank_for_address(_start); }



inline Object* Abstract_Object_Heap::object_from_chunk(Chunk* c) {
  // word after last object might cause object_from_chunk to wrap to low addresses
  return c < (Chunk*)end_objects() ? c->object_from_chunk() : NULL;
}


inline Object* Abstract_Object_Heap::object_from_chunk_without_preheader(Chunk* c) {
  // word after last object might cause object_from_chunk to wrap to low addresses
  return c < (Chunk*)end_objects() ? c->object_from_chunk_without_preheader() : NULL;
}


inline Object* Abstract_Object_Heap::next_object(Object* obj) {
  return object_from_chunk(obj->nextChunk());
}


inline Object* Abstract_Object_Heap::next_object_without_preheader(Object* obj) {
  return object_from_chunk_without_preheader(obj->nextChunk());
}


inline bool Abstract_Object_Heap::verify_address_in_heap(void* addr) {
  assert_always_msg(_start <= (Oop*)addr
                    &&                (Oop*)addr < _next,
                    "object is not a valid address");
  assert_always_msg((oop_int_t(addr) & (sizeof(Object*)-1)) == 0,
                    "object is not aligned");
  return true;
}

