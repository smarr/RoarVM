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
  static const int hs = Object::BaseHeaderSize/sizeof(Oop);
  oop_int_t contents = Object::make_free_object_header(bytes_including_header - hs);  
  DEBUG_STORE_CHECK((oop_int_t*)this, contents);  
  *(oop_int_t*)this = contents;
  // only used by GC and IT worries about coherence
  if (check_assertions) {
    oop_int_t filler = Oop::Illegals::made_free |  ((id & 15) << 24);
    assert(The_Memory_System()->contains(this));
    oopset_no_store_check(((Oop*)this) + hs, Oop::from_bits(filler), bytes_including_header/sizeof(Oop) - hs);
  }
}

