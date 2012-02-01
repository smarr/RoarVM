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
  volatile size_t free_area;

public:
  class Item {
  public:
    size_t size;
    
    size_t get_size() const {
      return (size & 0xFFFFFFFE);
    }
    
    void set_size(size_t sz) {
      assert(sz % sizeof(void*) == 0); // make sure that we are aligned
      size = sz;
    }
    
    bool is_actually_free_item() const {
      return (size & 1);
    }
    
    void become_free() {
      size = size | 1 ;
    }

    bool become_used() {
      size = (size & 0xFFFFFFFE);
    }

    /* For used objects we do not need next/prev,
       so, we just use that for the content. */
    void* get_content() {
      return (void*)&next;
    }
    
    static Item* from_content(void* value) {
      return (Item*)((intptr_t)value - offsetof(Item, next));
    }
    
    /** Pad for word alignment, but also make sure
     that the return items are never smaller than the item we use to manage
     the free spots in the heap. */
    static inline size_t manage_and_pad(size_t sz) {
      const size_t with_overhead = offsetof(Item, next) + sz;
      const size_t padded = pad(with_overhead);
      
      return max(padded, sizeof(Item));
    }
    
    static inline size_t pad(size_t sz) {
      return sz + ((sizeof(void*) - (sz % sizeof(void*))) % sizeof(void*));
    }

    Item* next;
    Item* prev;
  };
   
   
private:
  Item* volatile free_list;

  OS_Interface::Mutex mtx;

public:
  Interprocess_Allocator(void* const allocation_area, size_t size)
  : allocation_area(allocation_area), 
    size(size),
    free_area(size),
    num_allocations(0),
    sum_allocations(0),
    num_frees(0),
    num_allocated_chunks(0) {
    
    if (Debugging)
      invalidate_memory();
      
    initalize_allocator();
  }

  void* allocate(size_t sz) {
    if (sz == 0)
      return NULL;
    
    size_t managed_and_padded = Item::manage_and_pad(sz);
    void* result = NULL;
    
    if (managed_and_padded <= free_area) {
      OS_Interface::mutex_lock(&mtx);
      
      if (managed_and_padded <= free_area) {
        result = allocate_chunk(managed_and_padded);
        
        if (result) {
          num_allocations += 1;
          num_allocated_chunks += 1;
          sum_allocations += sz;
          
          free_area -= managed_and_padded;
        }
      }
      
      OS_Interface::mutex_unlock(&mtx);    
    }
    
    return result;
  }
  
  void* allocate_zeroed(size_t sz) {
    void* result = allocate(sz);
    memset(result, 0, sz);
    return result;
  }
  
  void* allocate_elements(size_t num_elements, size_t element_size) {
    return allocate_zeroed(num_elements * Item::pad(element_size));
  }
  
  void free(void* item) {
    assert(   item >= allocation_area
           && ((uintptr_t)item < (uintptr_t)allocation_area + size));
    
    OS_Interface::mutex_lock(&mtx);
    
    Item* freed_item = (Item*)Item::from_content(item);
    
    size_t additional_free_size = freed_item->get_size();
    
    freed_item->become_free();
    freed_item->prev = NULL;
    
    Item* following_item = (Item*)((intptr_t)freed_item + freed_item->get_size());
    
    if (((uintptr_t)following_item < (uintptr_t)allocation_area + size)
        && following_item->is_actually_free_item()) {
      merge_free_items(freed_item, following_item);
    }
    else {
      /* just put it infront of the free list */
      freed_item->next = free_list;
      if (free_list) {
        free_list->prev = freed_item;
      }
      
      free_list = freed_item;
    }
    
    free_area += additional_free_size;
    
    num_frees += 1;
    num_allocated_chunks -= 1;
    
    OS_Interface::mutex_unlock(&mtx);
  }
  
private:
  void merge_free_items(Item* const freed_item, Item* const following_item) {
    freed_item->set_size(freed_item->get_size() + following_item->get_size());
    freed_item->next = following_item->next;
    
    Item* prev_item_in_list = following_item->prev;
    freed_item->prev = prev_item_in_list;
    
    /* NULL out memory, is now in the middle of the resulting merged element */
    following_item->next = NULL;
    following_item->prev = NULL;
    following_item->set_size(0);
    
    if (prev_item_in_list) {
      prev_item_in_list->next = freed_item;
    }      
    else {
      assert_eq(following_item, free_list,
                "In case there is no previous item, "
                "we should be at the head of the free list.");
      free_list = freed_item;
    }
  }

/** For Debugging */
public:
  void debug_print_free_list() {}

  volatile uint32_t num_allocations;
  volatile uint32_t sum_allocations;
  
  volatile uint32_t num_frees;
  volatile uint32_t num_allocated_chunks;

/** Private Implementation */
private:
  void invalidate_memory() {
    memset(allocation_area, 0xee, size);
  }
  
  void initalize_allocator() {
    OS_Interface::mutex_init_for_cross_process_use(&mtx);
    
    free_list = (Item*)allocation_area;
    free_list->set_size(size);
    free_list->become_free();
    free_list->next = NULL;
    free_list->prev = NULL;
  }
  
  void* allocate_chunk(size_t managed_and_padded_size) {
    Item* result = NULL;
    assert(free_area >= managed_and_padded_size);  // should have been checked already
    
    Item* current = free_list;
    
    // first fit: find the first free spot that is big enough
    while (current != NULL && current->get_size() < managed_and_padded_size) {
      current = current->next;
    }
    
    // Looks like the heap is to fragmented
    if (current == NULL)
      return NULL;
    
    assert(current->get_size() >= managed_and_padded_size);
    result = current;
    
    // now two possible cases, either the found free_item is a perfect fit, or we split it up
    
    if (managed_and_padded_size + (2 * sizeof(Item)) >= current->get_size()) {
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
      size_t chunk_size = current->get_size();
      Item* next = current->next;
      Item* prev = current->prev;
      
      result->set_size(managed_and_padded_size);
      
      Item* remaining_chunk = (Item*)((intptr_t)current + managed_and_padded_size);
      remaining_chunk->set_size(chunk_size - managed_and_padded_size);
      remaining_chunk->become_free();
      remaining_chunk->prev = prev;
      remaining_chunk->next = next;
      
      if (free_list == current) {
        free_list = remaining_chunk;
      }
      else {
        prev->next = remaining_chunk;
      }
    }
    
    assert_eq(current->get_size(), result->get_size(),
              "Unexpectedly used and free items do not match "
              "on the size attribute, needs to be fixed.");
    
    result->become_used();
    return result->get_content();
  }

};
