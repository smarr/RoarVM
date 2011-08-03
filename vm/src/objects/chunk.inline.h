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


inline void Chunk::make_free_object(oop_int_t bytes_including_header, int id) {
  oop_int_t bytes_excluding_preheader = bytes_including_header - preheader_byte_size;
  assert(bytes_excluding_preheader > 0);
  
  static const int hs = Object::BaseHeaderSize/sizeof(Oop);
  
  // skip over the preheader and initialize it later properly
  Object* const first_object_header_word = (Object*)((char*)this + preheader_byte_size);
  
  oop_int_t contents = Object::make_free_object_header(bytes_excluding_preheader);
  
  DEBUG_STORE_CHECK((oop_int_t*)first_object_header_word, contents);  
  *(oop_int_t*)first_object_header_word = contents;
  
  // only used by GC and IT worries about coherence
  if (check_assertions) {
    oop_int_t filler = Oop::Illegals::made_free |  ((id & 15) << 24);
    assert(The_Memory_System()->contains(first_object_header_word));
    oopset_no_store_check(((Oop*)first_object_header_word) + hs, Oop::from_bits(filler), bytes_excluding_preheader/sizeof(Oop) - hs);
    
    # if Has_Preheader
    ((Preheader*)this)->mark_all_preheader_words_free_for_debugging();
    # endif
  }
}

inline Object_p Chunk::fill_in_after_allocate(oop_int_t byteSize,
                                              oop_int_t hdrSize,
                                              oop_int_t baseHeader,
                                              Oop classOop,
                                              oop_int_t extendedSize,
                                              bool doFill,
                                              bool fillWithNil) {
  const int my_rank = Logical_Core::my_rank();
  if (check_many_assertions  &&  hdrSize > 1)
    classOop.verify_oop();
  // since new allocs are in read_write heap, no need to mark this for moving to read_write
  assert(The_Memory_System()->contains(this));
  
  oop_int_t* headerp = (oop_int_t*)first_byte_after_preheader_from_chunk();
  Object_p    newObj = (Object_p)(Object*)&headerp[hdrSize - 1];
  assert(The_Memory_System()->is_address_read_write(this)); // not going to bother with coherence
  
  Multicore_Object_Heap* h = The_Memory_System()->heaps[my_rank][Memory_System::read_write];
  assert(h == my_heap()  ||  Safepoint_for_moving_objects::is_held());
  
  if (hdrSize == 3) {
    oop_int_t contents = extendedSize     |  Header_Type::SizeAndClass;
    DEBUG_STORE_CHECK(headerp, contents);
    *headerp++ = contents;
    
    contents = classOop.bits()  |  Header_Type::SizeAndClass;
    DEBUG_STORE_CHECK(headerp, contents);
    *headerp++ = contents;
    
    h->record_class_header((Object*)headerp, classOop);
    
    contents   = baseHeader       |  Header_Type::SizeAndClass;
    DEBUG_STORE_CHECK(headerp, contents);
    *headerp   = contents;
  }
  else if (hdrSize == 2) {
    oop_int_t contents = classOop.bits()  |  Header_Type::Class;
    DEBUG_STORE_CHECK(headerp, contents);
    *headerp++ = contents;
    
    h->record_class_header((Object*)headerp, classOop);
    
    contents   = baseHeader       |  Header_Type::Class;
    DEBUG_STORE_CHECK(headerp, contents);
    *headerp   = contents;
  }
  else {
    assert_eq(hdrSize, 1, "");
    
    oop_int_t contents   = baseHeader       |  Header_Type::Short;
    DEBUG_STORE_CHECK(headerp, contents);
    *headerp   = contents;
  }
  assert_eq((void*)newObj, (void*)headerp, "");
  
  The_Memory_System()->object_table->allocate_oop_and_set_preheader(newObj, my_rank  COMMA_TRUE_OR_NOTHING);
  
  
  //  "clear new object"
  if (!doFill)
    ;
  else if (fillWithNil) // assume it's an oop if not null
    h->multistore((Oop*)&headerp[1],
                  (Oop*)&headerp[byteSize >> ShiftForWord],
                  The_Squeak_Interpreter()->roots.nilObj);
  else {
    DEBUG_MULTISTORE_CHECK( &headerp[1], 0, (byteSize - sizeof(*headerp)) / bytes_per_oop);
    bzero(&headerp[1], byteSize - sizeof(*headerp));
  }
  
  The_Memory_System()->enforce_coherence_after_store_into_object_by_interpreter(this, byteSize);
  
  if (check_assertions) {
    newObj->okayOop();
    newObj->hasOkayClass();
  }
  return newObj;
}

Multicore_Object_Heap* Chunk::my_heap() {
  return The_Memory_System()->heap_containing(this);
}

