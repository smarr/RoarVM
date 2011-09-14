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


/*
 A chunk is the unit used by the memory system, the heap to manage the memory.
 
 TODO: add information whether empty chunks are somehow abused to store data.
       and how it is encoded.
 */

class Chunk
# if !Work_Around_Extra_Words_In_Classes
: public Word_Containing_Object_Type
# endif

{
  # if Work_Around_Extra_Words_In_Classes
    # include "word_containing_object_type.h"
    # define chunk_header (*(oop_int_t*)this)
  # else
    oop_int_t chunk_header;
  # endif

 public:
  
  /**
   * Determine the start of the object, i.e., the position of the base header.
   *
   * See object.h for details.
   */
  Object* object_from_chunk() {
    Word_Containing_Object_Type* const after_preheader =
          (Word_Containing_Object_Type*)first_byte_after_preheader_from_chunk();
    
    
    return (Object*) & ((char*)after_preheader)[after_preheader->extra_header_bytes_without_preheader()];
  }

  /**
   * Determine the start of the object, i.e., the position of the base header.
   *
   * REMARK: this is used for chunks read from a standard image.
   *         In such a case, the chunks do NOT contain a preheader.
   *
   * See object.h for details.
   */
  Object* object_from_chunk_without_preheader() {
    // "Compute the oop of this chunk by adding its extra header bytes."
    return (Object*) & ((char*)this)[extra_header_bytes_without_preheader()];
  }
  
  inline void* first_byte_after_preheader_from_chunk() {
    return (char*)this + preheader_byte_size;
  }
  

  // ObjectMemory allocation
  Object_p fill_in_after_allocate(oop_int_t byteSize, oop_int_t hdrSize,
                                  oop_int_t baseHeader, Oop classOop, oop_int_t extendedSize,
                                  bool doFill = false,
                                  bool fillWithNil = false);
  
  void make_free_object(oop_int_t bytes_including_header, int id);
  
  inline Multicore_Object_Heap* my_heap();
};

