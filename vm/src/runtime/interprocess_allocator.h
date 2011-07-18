/******************************************************************************
 *  Copyright (c) 2008 - 2010 IBM Corporation and others.
 *  All rights reserved. This program and the accompanying materials
 *  are made available under the terms of the Eclipse Public License v1.0
 *  which accompanies this distribution, and is available at
 *  http://www.eclipse.org/legal/epl-v10.html
 * 
 *  Contributors:
 *    Stefan Marr, Vrije Universiteit Brussel - Initial Implementation
 ******************************************************************************/


/**
 * The Interprocess_Allocator is used to manage memory in a memory region
 * that can be shared between multiple process.
 */
class Interprocess_Allocator {
private:
  void*  const allocation_area;
  size_t const size;
  size_t free_area;

  typedef struct free_item {
    size_t size;
    struct free_item* next;
    struct free_item* prev;
  } free_item_t;
   
  typedef struct used_item {
    size_t size;
    char   content[0];
  } used_item_t;
   
  free_item_t* free_list;

  OS_Interface::Mutex mtx;

public:
  Interprocess_Allocator(void* const allocation_area, size_t size)
  : allocation_area(allocation_area), 
    size(size),
    free_area(size),
    num_allocations(0),
    sum_allocations(0) {
    
    if (Debugging)
      invalidate_memory();
      
    initalize_allocator();
  }

  void* allocate(size_t sz) {
    size_t managed_and_padded = pad_for_word_alignment(sz + sizeof(used_item_t));
  }
  
  void  free(void* item) {}

/** For Debugging */
public:
  void debug_print_free_list() {}

  uint32_t num_allocations;
  uint32_t sum_allocations;

/** Private Implementation */
private:
  void invalidate_memory() {
    memset(allocation_area, Oop::Illegals::uninitialized, size);
  }
  
  void initalize_allocator() {
    OS_Interface::mutex_init(&mtx); // TODO: init correctly for cross-process use
    
    free_list = (free_item_t*)allocation_area;
    free_list->size = size;
    free_list->next = NULL;
    free_list->prev = NULL;
  }

public:
  static inline size_t pad_for_word_alignment(size_t sz) {
    return sz + ((sizeof(void*) - (sz % sizeof(void*))) % sizeof(void*));
  }

};
