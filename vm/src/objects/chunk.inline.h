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
    
    ((Preheader*)this)->mark_all_preheader_words_free_for_debugging();
  }
}

