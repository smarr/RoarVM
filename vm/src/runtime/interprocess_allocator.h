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
    if (sz == 0)
      return NULL;
    
    size_t managed_and_padded = pad_for_word_alignment(sz + sizeof(used_item_t));
    void* result = NULL;
    
    if (managed_and_padded <= free_area) {
      OS_Interface::mutex_lock(&mtx);
      
      if (managed_and_padded <= free_area) {
        result = allocate_chunk(managed_and_padded);
        
        if (result) {
          num_allocations += 1;
          sum_allocations += sz;
          
          free_area -= managed_and_padded;
        }
      }
      
      OS_Interface::mutex_unlock(&mtx);    
    }
    
    return result;
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
    memset(allocation_area, 0xee, size);
  }
  
  void initalize_allocator() {
    OS_Interface::mutex_init_for_cross_process_use(&mtx);
    
    free_list = (free_item_t*)allocation_area;
    free_list->size = size;
    free_list->next = NULL;
    free_list->prev = NULL;
  }
  
  void* allocate_chunk(size_t managed_and_padded_size) {
    void* result = NULL;
    assert(free_area >= managed_and_padded_size);  // should have been checked already
    
    free_item_t* current = free_list;
    
    // first fit: find the first free spot that is big enough
    while (current != NULL && current->size < managed_and_padded_size) {
      current = current->next;
    }
    
    // Looks like the heap is to fragmented
    if (current == NULL)
      return NULL;
    
    assert(current->size >= managed_and_padded_size);
    
    // now two possible cases, either the found free_item is a perfect fit, or we split it up
    
    if (managed_and_padded_size + (2 * sizeof(free_item_t)) >= current->size) {
      // the current chunk is perfect, almost perfect
      // now just use it
      if (free_list == current) {
        // at the head of the list, not much to do
        free_list = current->next;
        if (free_list)
          free_list->prev = NULL;
      }
      else {
        // otherwise, we take the element out and relink the free list
        assert(current->prev);
        current->prev->next = current->next;
        if (current->next)
          current->next->prev = current->prev;
      }
    }
    else {
      // the current chunk is large enough to split it before using
      
      // Save the current content, might be overriden when allocation
      // requests are very small
      size_t chunk_size = current->size;
      free_item_t* next = current->next;
      free_item_t* prev = current->prev;
      
      current->size = managed_and_padded_size;
      
      free_item_t* remaining_chunk = (free_item_t*)((intptr_t)current + managed_and_padded_size);
      remaining_chunk->size = chunk_size - managed_and_padded_size;
      remaining_chunk->prev = prev;
      remaining_chunk->next = next;
      
      if (free_list == current) {
        free_list = remaining_chunk;
      }
      else {
        prev->next = remaining_chunk;
      }
    }
    
    assert_eq((current->size), ((used_item_t*)current)->size, "Unexpectedly the structs of used and free items do not match on the size attributed, needs to be fixed.");
    
    return &(((used_item_t*)current)->content);
  }

public:
  static inline size_t pad_for_word_alignment(size_t sz) {
    return sz + ((sizeof(void*) - (sz % sizeof(void*))) % sizeof(void*));
  }

};
