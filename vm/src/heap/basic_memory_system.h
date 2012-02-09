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


/**
 * The Memory_System manages the heap structure that is used for Smalltalk
 * objects.
 *
 * The heap is uses one segment per core.
 *
 * 
 * Conceptual Structure
 * --------------------
 *   A typical structure in memory could look like this:
 *
 *     +-----------------+
 *     |   read-write    |
 *     +----+----+----+--+
 *     | c1 | c2 | c3 |..|
 *     +----+----+----+--+
 *     0                 n
 *
 * Implementation Structure
 * ------------------------
 *
 * The heap is allocated with mmap.
 * See map_heap_memory(..)
 *
 * The memory allocation is done on the main core and all other cores
 * receive the base address via a message to map the same memory regions
 * into the same addresses.
 * For thread-based systems, this is only done on the main core, and the other
 * cores do not need to map in any memory.
 *
 * A temporary file is used to ensure that all cores are working on the same
 * memory.
 */
class Basic_Memory_System : public Abstract_Memory_System {
protected:
  static const int max_num_mutabilities = 1;
  static const int read_write = 0;
  
  int second_chance_cores_for_allocation[max_num_mutabilities];  // made threadsafe to increase the reliability of the value
  
public:
  Multicore_Object_Heap* heaps[Max_Number_Of_Cores][max_num_mutabilities];

  char* get_memory_base() const {
    return read_write_memory_base;
  }

public:
  Basic_Memory_System() : Abstract_Memory_System() {}

  struct init_buf {
    int32 snapshot_bytes, sws, fsf, lastHash;
    char* read_write_memory_base;
    u_int32 total_read_write_memory_size;
    u_int32 memory_per_read_write_heap;
    u_int32 log_memory_per_read_write_heap;
    int32 page_size;
    int32 main_pid;
    Multicore_Object_Table* object_table;
    struct Global_GC_Values* global_GC_values;
  };


  void initialize_from_snapshot(int32 snapshot_bytes, int32 sws, int32 fsf, int32 lastHash);

 
private:
  void set_page_size_used_in_heap();
  int how_many_huge_pages();
  void request_huge_pages(int);

  void map_heap_memory_in_one_request(int pid, size_t total, void* start_adress = NULL);
  
protected:
  void map_memory_on_helper(init_buf* ib);
  
  void initialize_main(init_buf* buffer);

  void map_heap_memory(int pid, size_t);

  void receive_heap(int i);
  
  void create_my_heaps(init_buf*);
  void init_values_from_buffer(init_buf*);
  void send_local_heap();
  
  
protected:

  Multicore_Object_Heap* local_heap_for_snapshot_object(Object* src_obj_wo_preheader) {
    return heaps[Logical_Core::my_rank()][read_write];
  }

public:
  
  void verify_local() {
    heaps[Logical_Core::my_rank()][read_write]->verify();
  }
  
  void zap_unused_portion() {
    heaps[Logical_Core::my_rank()][read_write]->zap_unused_portion();
  }
  
  void print_bytes_used() {
    FOR_ALL_RANKS(r)
      lprintf("%d: %d\n",
              r,
              heaps[r][read_write]->bytesUsed());
  }

 
  bool contains(void* p) const {
    return read_write_memory_base <= (char*)p  &&  (char*)p < read_write_memory_past_end;
  }
  
  int mutability_for_address(void* p) const {
    return read_write;
  }
  
  inline static int mutability_for_oop(Oop* /* oop */) {
    return read_write;
  }
  
  static inline bool mutability_from_bool(bool) {
    return read_write;
  }
  
  static inline int mutability_for_posibile_replication(Object*) {
    return read_write;
  }
  
  inline Multicore_Object_Heap* get_heap(int rank) const {
    return heaps[rank][read_write];
  }
  
  int rank_for_address(void* p) const {
    u_int32 delta  = (char*)p - read_write_memory_base;
    u_int32 result = delta >> log_memory_per_read_write_heap;
    assert(result ==  delta / memory_per_read_write_heap);

    assert(result < (u_int32)Logical_Core::group_size);
    return (int)result;
  }

  Multicore_Object_Heap* heap_containing(void* obj) {
    return heaps[rank_for_address(obj)][mutability_for_address(obj)];
  }

  void scan_compact_or_make_free_objects_here(bool compacting, Abstract_Mark_Sweep_Collector*);


public:
  bool moveAllToRead_MostlyHeaps() { /* Not supported, indicate failure. */ return false; }


  void push_heap_stats();
  
  void enforce_coherence_after_this_core_has_stored_into_all_heaps()    {}
  void enforce_coherence_in_whole_heap_after_store()                    {}
  void enforce_coherence_after_each_core_has_stored_into_its_own_heap() {}
  void invalidate_heaps_and_fence(bool) {
    OS_Interface::mem_fence();
  }


  void print();

  inline bool is_address_read_mostly(void* p) const {
    return false;
  }
  
  inline bool is_address_read_write(void* p) const { 
    return true;
  }
};

