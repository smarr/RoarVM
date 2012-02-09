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


#ifndef __INTERPROCESS_ALLOCATOR_H__
#define __INTERPROCESS_ALLOCATOR_H__

#ifndef debug_printf
#define debug_printf printf
#endif

#ifndef assert_eq
#define assert_eq(a, b, msg) assert((a) == (b))
#endif

/* RoarVM code should define it as debug_printer->printf */

/**
 * The Interprocess_Allocator is used to manage memory in a memory region
 * that can be shared between multiple process.
 * 
 * The free list is a simple circular doubly-linked list in the style of Ungar.
 */
class Interprocess_Allocator {
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
    
    Item* next_in_memory() {
      return (Item*)((intptr_t)this + get_size());
    }

    Item* next;
    Item* prev;
  };
  
private:
  class Shared_Allocator_Data {
  public:
    void*  const allocation_area;
    size_t const size;
    
    Item* volatile free_list;
    Item  free_item; /* The terminator element in the circular free list */
  
    volatile size_t free_area;

    OS_Interface::Mutex mtx;
    
    volatile uint32_t num_allocations;
    volatile uint32_t sum_allocations;
    
    volatile uint32_t num_frees;
    volatile uint32_t num_allocated_chunks;
    
    Shared_Allocator_Data(void* shared_memory, size_t size)
    : allocation_area((void*)((intptr_t)shared_memory + (off_t)round_up_to_power_of_two(sizeof(Shared_Allocator_Data)))),
      size(size - round_up_to_power_of_two(sizeof(Shared_Allocator_Data))),
      free_list(NULL),
      free_area(size - round_up_to_power_of_two(sizeof(Shared_Allocator_Data))),
      num_allocations(0),
      sum_allocations(0), num_frees(0), num_allocated_chunks(0) {
      if (Debugging)
        invalidate_memory();
      initalize_allocator(allocation_area, this->size);
    }
    
    void* operator new (size_t, void* place) {
      return place;
    }
  private:
    void* operator new(size_t) {
      fatal("Not supported. Make sure that the instance "
            "is allocated with the placement new");
    }
    
    void invalidate_memory() {
      memset(allocation_area, 0xee, size);
    }
    
    void initalize_allocator(void* allocation_area, size_t size) {
      OS_Interface::mutex_init_for_cross_process_use(&mtx);
      
      free_item.set_size(0);
      free_item.become_free();
      
      free_list = (Item*)allocation_area;
      free_list->set_size(size);
      free_list->become_free();
      
      free_list->next = &free_item;
      free_list->prev = &free_item;
      
      free_item.next = free_list;
      free_item.prev = free_list;
    }
  };

  Shared_Allocator_Data* const shared;

public:
  
  /**
   * Get a padded size to be sure the requested memory size is available
   * to the applications.
   */
  static size_t pad_request_size_for_allocation_area(size_t size) {
    return size + round_up_to_power_of_two(sizeof(Shared_Allocator_Data));
  }
  
  /**
   * This constructor is supposed to be used on freshly allocated memory.
   */
  Interprocess_Allocator(void* const shared_memory, size_t size)
  : shared((Shared_Allocator_Data* const)shared_memory) {
    new (shared_memory) Shared_Allocator_Data(shared_memory, size);
  }
  
  /**
   * This constructor is supposed to be used on memory that is already under
   * control of an Interprocess_Allocator instance.
   */
  Interprocess_Allocator(void* const shared_memory)
  : shared((Shared_Allocator_Data* const)shared_memory) {}

  uint32_t num_allocated_chunks() {
    return shared->num_allocated_chunks;
  }
  
  void* allocate(size_t sz) {
    if (sz == 0)
      return NULL;
    
    size_t managed_and_padded = Item::manage_and_pad(sz);
    void* result = NULL;
    
    if (managed_and_padded <= shared->free_area) {
      OS_Interface::mutex_lock(&shared->mtx);
      check_internal_consistency();      

      if (managed_and_padded <= shared->free_area) {
        size_t allocated_size = 0;
        result = allocate_chunk(managed_and_padded, allocated_size);

        if (result) {
          shared->num_allocations += 1;
          shared->num_allocated_chunks += 1;
          shared->sum_allocations += sz;
          
          shared->free_area -= allocated_size;
        }
      }
      
      check_internal_consistency();
      OS_Interface::mutex_unlock(&shared->mtx);    
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
    assert(   item >= shared->allocation_area
           && ((uintptr_t)item < (uintptr_t)shared->allocation_area + shared->size));
    
    OS_Interface::mutex_lock(&shared->mtx);
    check_internal_consistency();

    Item* freed_item = (Item*)Item::from_content(item);
    
    size_t additional_free_size = freed_item->get_size();
    
    Item* following_item = freed_item->next_in_memory();
    
    if (((uintptr_t)following_item < (uintptr_t)shared->allocation_area + shared->size)
        && following_item->is_actually_free_item()) {
      merge_free_items(freed_item, following_item);
    }
    else {
      /* just put it infront of the free list */
      freed_item->next = shared->free_list;
      freed_item->prev = shared->free_list->prev;
      
      shared->free_list->prev->next = freed_item;
      shared->free_list->prev = freed_item;

      shared->free_list = freed_item;
    }
    freed_item->become_free();
    
    shared->free_area += additional_free_size;
    
    shared->num_frees += 1;
    shared->num_allocated_chunks -= 1;

    check_internal_consistency();
    OS_Interface::mutex_unlock(&shared->mtx);
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
    
    prev_item_in_list->next = freed_item;
    
    if (following_item == shared->free_list) {
      shared->free_list = freed_item;
    }
    
    freed_item->next->prev = freed_item;
  }

/** For Debugging */
public:
  void debug_print_free_list() {}
  void debug_print_full_heap() {
    Item* current = (Item*)shared->allocation_area;
    
    debug_printf("\n\nInterprocess_Allocator Heap:\n");

    size_t seen_free = 0;
    size_t seen_used = 0;
    while (current && ((uintptr_t)current < (uintptr_t)shared->allocation_area + shared->size)) {
      debug_printf("\t[%s] size: %lu \tstart: %p\n", current->is_actually_free_item() ? "FREE" : "used", current->get_size(), current);
      
      if (current->is_actually_free_item()) {
        seen_free += current->get_size();
      }
      else {
        seen_used += current->get_size();
      }      
      current = current->next_in_memory();
    }

    debug_printf("\nseen free: %lu\n", seen_free);
    debug_printf("seen used: %lu\n", seen_used);
    
    debug_printf("size: %lu\n", shared->size);
    debug_printf("free area: %lu\n", shared->free_area);
    debug_printf("num allocations: %u\n", shared->num_allocations);
    debug_printf("sum allocations: %u\n", shared->sum_allocations);
    debug_printf("num frees: %u\n", shared->num_frees);
    debug_printf("num allocated chunks: %u\n", shared->num_allocated_chunks);
  }
  
  void check_internal_consistency() {
    if (!check_many_assertions)
      return;
    
    size_t free_list_size = check_consistency_of_free_list_and_get_size();
    size_t free_heap_size = check_free_size_in_heap();
    assert(free_list_size == free_heap_size);
    assert(shared->free_area == free_list_size);
  }
  
  size_t check_consistency_of_free_list_and_get_size() {
    size_t free_size = 0;
    Item* current = shared->free_list;
    Item* prev = &shared->free_item;
    
    while (current != &shared->free_item) {
      assert(current->is_actually_free_item());
      assert(current->prev == prev); /* back point works */
      free_size += current->get_size();
      prev = current;
      current = current->next;
    }
    return free_size;
  }
  
  size_t check_free_size_in_heap() {
    Item* current = (Item*)shared->allocation_area;
    
    size_t seen_free = 0;
    size_t seen_used = 0;
    while (current && ((uintptr_t)current < (uintptr_t)shared->allocation_area + shared->size)) {     
      if (current->is_actually_free_item()) {
        seen_free += current->get_size();
      }
      else {
        seen_used += current->get_size();
      }      
      current = current->next_in_memory();
    }
    
    return seen_free;
  }

/** Private Implementation */
private:
  
  void* allocate_chunk(size_t managed_and_padded_size, size_t& allocated_size) {
    assert(shared->free_area >= managed_and_padded_size);  // should have been checked already
    
    Item* current = shared->free_list;
    
    // first fit: find the first free spot that is big enough
    while (current != &shared->free_item && current->get_size() < managed_and_padded_size) {
      current = current->next;
    }
    
    // Looks like the heap is to fragmented
    if (current->get_size() < managed_and_padded_size)
      return NULL;
    
    assert(current->get_size() >= managed_and_padded_size);
    Item* result = current;
    
    // now two possible cases, either the found free_item is a perfect fit, or we split it up
    if (managed_and_padded_size + (2 * sizeof(Item)) >= current->get_size()) {
      // the current chunk is perfect, almost perfect
      // now just use it
      if (shared->free_list == current) {
        shared->free_list = current->next;
      }
      current->prev->next = current->next;
      current->next->prev = current->prev;
    }
    else {
      // the current chunk is large enough to split it before using
      
      // Save the current content, might be overriden when allocation
      // requests are very small
      size_t chunk_size = current->get_size();
      Item* const next = current->next;
      Item* const prev = current->prev;
      
      result->set_size(managed_and_padded_size);
      
      Item* remaining_chunk = result->next_in_memory();
      remaining_chunk->set_size(chunk_size - managed_and_padded_size);
      remaining_chunk->become_free();
      remaining_chunk->prev = prev;
      remaining_chunk->next = next;
      
      if (shared->free_list == current) {
        shared->free_list = remaining_chunk;
      }
      
      prev->next = remaining_chunk;
      next->prev = remaining_chunk;
    }
    
    assert_eq(current->get_size(), result->get_size(),
              "Unexpectedly used and free items do not match "
              "on the size attribute, needs to be fixed.");
    
    result->become_used();
    allocated_size = result->get_size();    
    return result->get_content();
  }

};

# endif // __INTERPROCESS_ALLOCATOR_H__
