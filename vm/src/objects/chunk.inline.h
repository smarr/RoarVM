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
  oop_int_t bytes_exluding_preheader = bytes_including_header - preheader_byte_size;
  
  static const int hs = Object::BaseHeaderSize/sizeof(Oop);
  
  // skip over the preheader and initialize it later properly
  Object* const object_start = (Object*)((char*)this + preheader_byte_size);
  
  oop_int_t contents = Object::make_free_object_header(bytes_exluding_preheader);
  
  DEBUG_STORE_CHECK((oop_int_t*)object_start, contents);  
  *(oop_int_t*)object_start = contents;
  
  // only used by GC and IT worries about coherence
  if (check_assertions) {
    oop_int_t filler = Oop::Illegals::made_free |  ((id & 15) << 24);
    assert(The_Memory_System()->contains(object_start));
    oopset_no_store_check(((Oop*)object_start) + hs, Oop::from_bits(filler), bytes_exluding_preheader/sizeof(Oop) - hs);
    
    ((Preheader*)this)->init_extra_preheader_word();
    // and set the backpointer to zero, should be like this:
    // object_start->set_backpointer_word(0);
    // but set_backpointer_word relies on a proper object, thus, do it directly:
    *(int*)this = 0;
    // STEFAN: a zero backpointer does not really make sense, but it is not 
    // properly defined when it is not a proper backpointer at the moment,
    // thus, I hope it breaks early enough if 0 is a problem
  }
}

