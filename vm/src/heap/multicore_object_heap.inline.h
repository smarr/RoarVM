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


inline void Multicore_Object_Heap::store_enforcing_coherence(oop_int_t* p, oop_int_t x, Object_p dst_obj_to_be_evacuated_or_null) {
  if (is_read_write()) {
    DEBUG_STORE_CHECK(p, x);
    *p = x;
  }
  else The_Memory_System()->store_enforcing_coherence(p, x, dst_obj_to_be_evacuated_or_null);
}

inline void Multicore_Object_Heap::store_enforcing_coherence(Oop* p, Oop x, Object_p dst_obj_to_be_evacuated_or_null) {
  store_enforcing_coherence((oop_int_t*)p, x.bits(), dst_obj_to_be_evacuated_or_null);
}

inline void Multicore_Object_Heap::store_bytes_enforcing_coherence(void* dst, const void* src, int nbytes, Object_p dst_obj_to_be_evacuated_or_null) {
  if (is_read_write())  {
    DEBUG_MULTIMOVE_CHECK( dst, src, nbytes / bytes_per_oop);
    memmove(dst, src, nbytes);
  }
  else The_Memory_System()->store_bytes_enforcing_coherence(dst, src, nbytes, dst_obj_to_be_evacuated_or_null);
}



inline Object_p Multicore_Object_Heap::allocate(oop_int_t byteSize, oop_int_t hdrSize,
                 oop_int_t baseHeader, Oop classOop, oop_int_t extendedSize,
                 bool doFill,
                 bool fillWithNil) {

  /* "Allocate a new object of the given size and number of header words.
      (Note: byteSize already includes space for the base header word.)
      Initialize the header fields of the new object and fill the remainder of
      the object with the given value.
      
      May cause a GC" */

  // "remap classOop in case GC happens during allocation"
  if (hdrSize > 1) The_Squeak_Interpreter()->pushRemappableOop(classOop);
  oop_int_t hdrSize_with_preheader = hdrSize + preheader_oop_size ;
  int total_bytes = byteSize  +  (hdrSize_with_preheader - 1) * bytesPerWord;
  
  Chunk* chunk = allocateChunk_for_a_new_object_and_safepoint_if_needed(total_bytes);
  
  Safepoint_Ability sa(false); // from here on, no GCs!
  
  Oop remappedClassOop = hdrSize > 1  ?  The_Squeak_Interpreter()->popRemappableOop() : Oop::from_int(0);
  Chunk* saved_next = !check_assertions ? NULL : (Chunk*) chunk->my_heap()->end_objects();
  Object_p newObj = chunk->fill_in_after_allocate(byteSize, hdrSize, baseHeader,
                                                 remappedClassOop, extendedSize, doFill, fillWithNil);
  assert_eq(newObj->nextChunk(), saved_next, "allocate bug: did not set header of new oop correctly");
  

  return newObj;
}


inline Chunk* Multicore_Object_Heap::allocateChunk_for_a_new_object_and_safepoint_if_needed(int total_bytes) {
  Safepoint_for_moving_objects* sp = NULL;
  if (The_Memory_System()->rank_for_address(_next) != Logical_Core::my_rank()) 
    sp = new Safepoint_for_moving_objects("inter-core allocate");

  Chunk* chunk = allocateChunk_for_a_new_object(total_bytes);
  
  if (sp != NULL) 
    delete sp;

  return chunk;
}


inline Chunk* Multicore_Object_Heap::allocateChunk_for_a_new_object(oop_int_t total_bytes) {
  // Since interpreter EXPECTS GC at this point (only if the SafepointAbility is able),
  // can flush objects from local to global heap,
  // or even resort to allocation in global heap.
  return allocateChunk(total_bytes);
}


inline int32 Multicore_Object_Heap::newObjectHash() {
  // "Answer a new 16-bit pseudo-random number for use as an identity hash."
  return lastHash = (13849 + (27181 * (lastHash + Logical_Core::my_rank()))) & 65535;
}

inline Object_p Multicore_Object_Heap::object_address_unchecked(Oop x) {
  return x.as_object_unchecked();
}


# if Use_Object_Table

inline bool Multicore_Object_Table::Entry::is_used() {
  Object* ow = word()->obj();
  return The_Memory_System()->contains(ow);
}

inline bool Multicore_Object_Table::probably_contains(void* p) const {
  if (The_Memory_System()->contains(p)) return false;
  FOR_ALL_RANKS(r)
    if (lowest_address[r] <= p  &&  p  < lowest_address_after_me[r])
      return true;
  return false;
}

# endif

