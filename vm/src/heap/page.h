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

/**
 *  The Page struct represents a "page" that can be allocated to a Core's Object Heap by 
 *  the Memory System (i.e., on request). It provides a limited amount of memory for an 
 *  Object Heap, in which the latter can allocate space to store new objects.
 *  
 *  Note: This struct is only useful for free pages (i.e., pages that have not been allocated
 *        to an Object Heap). The latter overwrites the contents of the page which
 *        renders the struct's accessors unusable.
 *  
 *  See Memory_System.h for more information.
 */
 
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
  
  Object* firstObject() {
    return ((Chunk*)this)->object_from_chunk();
  }  
  
  int pageNumber() {
    return this-(Page*)The_Memory_System()->firstPage(); 
  }
  
  size_t size() {
    return preheader_byte_size 
           + ((Object*)((char*)this + preheader_byte_size))->sizeOfFree();
  }
} Page;     

# define FOR_EACH_PAGE(page) \
for ( Page * page = (Page*)The_Memory_System()->firstPage(); \
      (char*)page < The_Memory_System()->heap_past_end; \
             page++ )

# define FOR_EACH_OBJECT_IN_PAGE(page, object) \
for ( Object* object = page->firstObject(); \
      (Page*)object < (page + 1); \
      object = object->nextObject() )

# define FOR_EACH_OBJECT_IN_UNPROTECTED_PAGE(page, object) \
for ( Object* object = page->firstObject(); \
      (Page*)object < (page + 1); \
      object = object->nextObject_unprotected() )


/**
 *  The LPage struct represents an entry for the aforementioned "page" in a Liveness Array. 
 *  The Liveness Array is employed by both the Memory System and the Parallel_GC_Thread to 
 *  support parallel garbage collection. In general, it keeps track of which pages are 
 *  allocated and how much of their capacity is actually live (i.e., reachable by the GC's 
 *  mark phase).
 *
 *  See Parallel_GC_Thread.h/cpp for more information.
 */

typedef struct LPage {
  int liveBytes;
    
  void setAllocated(bool v) {
    if(v) liveBytes = 0;
    else  liveBytes = Mega + 1;
  }
   
  bool isAllocated() { 
    return liveBytes < Mega; 
  }
   
  void addLiveBytes(int n) {
    if(liveBytes > Mega)
      fatal("Should not happen");
    else {
      liveBytes += n;
      if(liveBytes > Mega) 
        liveBytes = Mega;
    }   
  }    
} LPage;