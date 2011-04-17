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


class Chunk
# if !Work_Around_Extra_Words_In_Classes
: public Word_Containing_Object_Type
# endif

{
  # if Work_Around_Extra_Words_In_Classes
  WORD_CONTAINING_OBJECT_TYPE_MEMBERS
    # define chunk_header (*(oop_int_t*)this)
  # else
    oop_int_t chunk_header;
  # endif

 public:
  Object* object_from_chunk() {
    // "Compute the oop of this chunk by adding its extra header bytes."
    return (Object*) & ((char*)this)[extra_header_bytes()];
  }

  Object* object_from_chunk_without_preheader() {
    // "Compute the oop of this chunk by adding its extra header bytes."
    return (Object*) & ((char*)this)[extra_header_bytes_without_preheader()];
  }

  void make_free_object(oop_int_t bytes_including_header, int id);
};

