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


inline void Basic_Memory_System::putLong(int32 x, FILE* f) {
  if (The_Squeak_Interpreter()->successFlag) {
    if (fwrite(&x, sizeof(x), 1, f) != 1) {
      perror("write: ");
      The_Squeak_Interpreter()->primitiveFail();
    }
  }
}


inline Object* Basic_Memory_System::allocate_chunk_on_this_core_for_object_in_snapshot(Multicore_Object_Heap* h, Object* src_obj_wo_preheader) {
  Chunk* c = h->allocateChunk(src_obj_wo_preheader->total_byte_size());
  Object* obj = (Object*)&((char*)c)[src_obj_wo_preheader->extra_header_bytes()];
  return obj;
}

