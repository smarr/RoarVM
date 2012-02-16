/******************************************************************************
 *  Copyright (c) 2008 - 2012 IBM Corporation and others.
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
 * The heap is allocated per core and the memory of each core is divided
 * into a read-mostly and a read-write part.
 *
 * On NUMA architectures, we want to be able to manage cache coherency
 * ourselves in certain situations.
 * Currently, we use an extra heap region to manage objects that are
 * mostly read, but seldomly updated. This region uses only software-based
 * coherency to improve the performance properties.
 *
 * 
 * Conceptual Structure
 * --------------------
 *
 *   A typical structure in memory could look like this:
 *
 *     +-----------------+-----------------+
 *     |   read-mostly   |   read-write    |
 *     +----+----+----+--+----+----+----+--+
 *     | c1 | c2 | c3 |..| c1 | c2 | c3 |..|
 *     +----+----+----+--+----+----+----+--+
 *     0                n/2                n
 *
 * Implementation Structure
 * ------------------------
 *
 * The heap is allocated with mmap in two parts but at consecutive addesses.
 * The first region is allocated for the read-mostly heap, and
 * the second region is allocated for the read-write heap.
 * See map_read_write_and_read_mostly_memory(..)
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
class Read_Mostly_Memory_System : public Abstract_Memory_System {
protected:
  static const int max_num_mutabilities = 2;
  static const int read_mostly = 0;
  static const int read_write  = 1;

  int second_chance_cores_for_allocation[max_num_mutabilities];  // made threadsafe to increase the reliability of the value

public:
  Multicore_Object_Heap* heaps[Max_Number_Of_Cores][max_num_mutabilities];

private:

  
  char * read_mostly_memory_base,  * read_mostly_memory_past_end;
  
  void map_heap_memory_separately(int pid, size_t grand_total,
                                  size_t inco_size, size_t co_size,
                                  char* requested_rw_base,
                                  char* requested_rm_base);
  void map_read_write_and_read_mostly_memory(int pid, size_t total_read_write_memory_size,
                                             size_t total_read_mostly_memory_size,
                                             char* requested_rw_base,
                                             char* requested_rm_base);
  
  static u_int32 memory_per_read_mostly_heap; // threadsafe readonly, will always be power of two
  static u_int32 log_memory_per_read_mostly_heap; // threadsafe readonly
  

protected:

  Multicore_Object_Heap* local_heap_for_snapshot_object(Object* /* src_obj_wo_preheader */) {
    // Used to be is_suitable_for_replication() before multithreading, but now
    // need to exclude certainly classes that we don't know till AFTER reading the snapshot -- dmu 3/30/09
    // So, put everything in read_write, and let image move objects to read_mostly later. -- dmu 5/25/10
    // bool repl =  Object::is_suitable_for_replication();
    const bool repl = false;
    
    if (repl)
      return heaps[Logical_Core::my_rank()][read_mostly];
    else
      return heaps[Logical_Core::my_rank()][read_write];
  }

  void set_page_size_used_in_heap();
  
  void map_heap_memory_in_one_request(int pid, size_t grand_total,
                                      size_t inco_size, size_t co_size, char* requested_rm_base);
protected:
  void receive_heap(int i);
  void send_local_heap();

public:
  Read_Mostly_Memory_System() : Abstract_Memory_System() {
    for (int rank = 0;  rank < Max_Number_Of_Cores;  ++rank)
      for (int mutability = 0;  mutability < max_num_mutabilities;  ++mutability)
        heaps[rank][mutability] = NULL;
  }
  
  char* get_memory_base() const {
    return read_mostly_memory_base;
  }

  bool contains(void* p) const {
    return read_mostly_memory_base <= (char*)p  &&  (char*)p < read_write_memory_past_end;
  }

  int mutability_for_address(void* p) const {
    return is_address_read_write(p) ? read_write : read_mostly;
  }
  
  inline static int mutability_for_oop(Oop* oop);
  
  static inline bool mutability_from_bool(bool value) {
    return value
            ? read_write
            : read_mostly;
  }

  static inline int mutability_for_posibile_replication(Object* obj) {
    return !obj->is_suitable_for_replication()
                    ? read_write
                    : read_mostly;
  }

  inline Multicore_Object_Heap* get_heap(int rank) const {
    return heaps[rank][read_write];
  }
  
  Multicore_Object_Heap* heap_containing(void* obj) {
    return heaps[rank_for_address(obj)][mutability_for_address(obj)];
  }

  int rank_for_address(void* p) const {
    bool is_rw = is_address_read_write(p);
    u_int32 delta  = (char*)p - (is_rw ? read_write_memory_base : read_mostly_memory_base);
    u_int32 result = delta >> (is_rw ? log_memory_per_read_write_heap : log_memory_per_read_mostly_heap);
    assert(result ==  delta / (is_rw ? memory_per_read_write_heap : memory_per_read_mostly_heap));

    assert(result < (u_int32)Logical_Core::group_size);
    return (int)result;
  }

  inline bool is_address_read_mostly(void* p) const {
    // don't use read_mostly_heap->contains(p) because that is slower, as it tests the bottom before the top
    // in the common case, p is in the read_write heap, which is above the read_mostly one.
    return (char*)p < read_mostly_memory_past_end;
  }
  
  inline bool is_address_read_write(void* p) const { 
    return !is_address_read_mostly(p);
  }

  void verify_local() {
    heaps[Logical_Core::my_rank()][read_mostly]->verify();
    heaps[Logical_Core::my_rank()][read_write ]->verify();
  }
  
  void zap_unused_portion() {
    heaps[Logical_Core::my_rank()][read_write ]->zap_unused_portion();
    heaps[Logical_Core::my_rank()][read_mostly]->zap_unused_portion();
  }
  
  void print_bytes_used() {
    FOR_ALL_RANKS(r)
      lprintf("%d: %d @ %d\n",
            r,
            heaps[r][read_write ]->bytesUsed(),
            heaps[r][read_mostly]->bytesUsed());
  }

  void enforce_coherence_after_each_core_has_stored_into_its_own_heap();

  void initialize_from_snapshot(int32 snapshot_bytes, int32 sws, int32 fsf, int32 lastHash);

  void print();
  
  void scan_compact_or_make_free_objects_here(bool compacting, Abstract_Mark_Sweep_Collector*);
  
  void enforce_coherence_in_whole_heap_after_store() {
    heaps[Logical_Core::my_rank()][read_mostly]->enforce_coherence_in_whole_heap_after_store();
  }
  
  void enforce_coherence_after_this_core_has_stored_into_all_heaps() {
    FOR_ALL_RANKS(i)
      heaps[i][read_mostly]->enforce_coherence_in_whole_heap_after_store();
  }
  
  void invalidate_heaps_and_fence(bool mine_too) {
    FOR_ALL_RANKS(i)
      if (mine_too  ||  i != Logical_Core::my_rank())
        heaps[i][read_mostly]->invalidate_whole_heap();

    OS_Interface::mem_fence();
  }
  
  bool moveAllToRead_MostlyHeaps();
  
  struct init_buf {
    Basic_Memory_System::init_buf base_buf;
    char*   read_mostly_memory_base;
    u_int32 total_read_mostly_memory_size;
    u_int32 memory_per_read_mostly_heap;
    u_int32 log_memory_per_read_mostly_heap;
  };

  void create_my_heaps(init_buf*);
  void init_values_from_buffer(init_buf*);
  void map_memory_on_helper(init_buf* ib);
  
  static int calculate_bytes_per_read_mostly_heap();  
  static int calculate_total_read_mostly_pages(int page_size);

private:
  void initialize_main(init_buf*);

protected:
  void push_heap_stats();
};
