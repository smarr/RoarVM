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
 *    Wouter Amerijckx, Vrije Universiteit Brussel - Parallel Garbage Collection
 ******************************************************************************/

static const int page_size = 1 * Mega; // bytes

typedef struct Page {
  char      preheader[preheader_byte_size]; // Preheader (preheader_byte_size bytes)

  oop_int_t header;                         // Free object header (4 bytes)
  Page*     next_free_page;                 // (4 bytes)
  
  char      rest[page_size - 8 - preheader_byte_size];

  void init() {
    initialize(page_size);  
  
    Page * next = this + 1;
    if( (char*)next < The_Memory_System()->heap_past_end )
      next_free_page = next;
    else
      next_free_page = NULL;
  }
  
  void initialize(size_t size) {
    ((Chunk*)this)->make_free_object_header(size,0);  
  }
  
  size_t size() {
    return 
    preheader_byte_size 
           + ((Object*)((char*)this + preheader_byte_size))->sizeOfFree();
  }
} Page;              