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


# if Use_ReadMostly_Heap
  typedef Read_Mostly_Memory_System Base_Memory_System;
# else
  typedef Basic_Memory_System       Base_Memory_System;
# endif

/**
 * The Memory_System is the configured instance that is used in the VM.
 * The typedef above changes the base class according to the configuration.
 * The class itself contains all code shared between the currently two
 * different options (with and without read-mostly heap).
 */
class Memory_System : public Base_Memory_System {
public:

  Memory_System() : Base_Memory_System() {}
  
  bool verify_if(bool);
  bool verify() { return verify_if(true); }

  
  void finished_adding_objects_from_snapshot();
  int32 adjust_for_snapshot(void* addr, u_int32* address_offsets) const {
    return (int32)addr - address_offsets[&heaps[rank_for_address(addr)][mutability_for_address(addr)] - &heaps[0][0]];
  }
  
  void enforce_coherence_before_each_core_stores_into_its_own_heap();
  void enforce_coherence_before_this_core_stores_into_all_heaps();
  
  void imageNameGet(Object_p, int);

  void flushExternalPrimitives();

  void scan_compact_or_make_free_objects_everywhere(bool compacting, Abstract_Mark_Sweep_Collector*);

  void initialize_main_from_buffer(void* buffer, size_t buffer_size);

  void print_heaps();
  
  bool sufficientSpaceAfterGC(oop_int_t, int);
  Logical_Core* coreWithSufficientSpaceToAllocate(oop_int_t bytes);

  void fullGC(const char*);
  void incrementalGC() {  if (check_assertions) lprintf("no incremental GC\n"); }
  void finalize_weak_arrays_since_we_dont_do_incrementalGC();

  void level_out_heaps_if_needed();
  bool shuffle_or_spread(int, int, bool, bool, bool);
  bool shuffle_or_spread_last_part_of_a_heap(Object*, int, int, bool, bool, bool);
  Multicore_Object_Heap* biggest_heap();
  int32 smallest_heap(int mutability);
  
  void set_second_chance_cores_for_allocation(int);
  
  u_int32 maxContiguousBytesLeft();
  u_int32 bytesLeft(bool);
  
  void  set_lowSpaceThreshold(int32);
  int32 get_lowSpaceThreshold()  {
    return heaps[Logical_Core::my_rank()][read_write]->get_lowSpaceThreshold();
  }

  
  // Cannot accept GC requests, defers to next BC boundary
  void enforce_coherence_before_store_into_object_by_interpreter(void* p, int nbytes, Object_p dst_obj_to_be_evacuated);

  void pre_cohere(void*, int);
  void post_cohere(void*, int);
  
  void  pre_cohere_object_table(void* p, int sz) {  pre_cohere(p, sz); }
  void post_cohere_object_table(void* p, int sz) { post_cohere(p, sz); }

  // OK to accept GC requests, so don't use inside of interpreter
  void  enforce_coherence_before_store(void* p, int nbytes) {
    assert(contains(p));
    if (is_address_read_mostly(p))  pre_cohere(p, nbytes);
  }

  void enforce_coherence_after_store_into_object_by_interpreter(void* /* p */, int /* nbytes */) {}
  void enforce_coherence_after_store(void* p, int nbytes)  {
    assert(contains(p));
    if (is_address_read_mostly(p)) post_cohere(p, nbytes);
  }

  void store_bytes_enforcing_coherence(void* dst, const void* src, int nbytes,   Object_p dst_obj_to_be_evacuated_or_null);
  
  void store_enforcing_coherence(Oop* p, Oop x, Object_p dst_obj_to_be_evacuated_or_null) {
    assert(contains(p));
    store_enforcing_coherence((oop_int_t*)p, x.bits(), dst_obj_to_be_evacuated_or_null);
  }
  // used when p may be either in the heap or in a C++ structure
  void store_enforcing_coherence_if_in_heap(Oop* p, Oop x, Object_p dst_obj_to_be_evacuated_or_null) {
    if (contains(p))
      store_enforcing_coherence((oop_int_t*)p, x.bits(), dst_obj_to_be_evacuated_or_null);
    else *p = x;
  }
  
  void store_2_enforcing_coherence(int32* p1, int32 i1, int32 i2,  Object_p dst_obj_to_be_evacuated_or_null);
  
  # define FOR_ALL_STORE_ENFORCING_COHERENCE_FUNCTIONS(template) \
    template(oop_int_t) \
    template(Object*) \
    template(int16) \
    template(char) \
    template(u_char)
  
  # define DCL_SEC(T) void store_enforcing_coherence(T* p, T x, Object_p dst_obj_to_be_evacuated_or_null);
  
  FOR_ALL_STORE_ENFORCING_COHERENCE_FUNCTIONS(DCL_SEC)
  
  # undef DCL_SEC

  inline Object* allocate_chunk_on_this_core_for_object_in_snapshot(Multicore_Object_Heap*, Object*);

  void handle_low_space_signals();

  void snapshotCleanUp();
  
  Oop get_stats(int);
  
  u_int32 bytesUsed();
  int32 max_lastHash();
  
  void compute_snapshot_offsets(u_int32 *offsets);
  void writeImageFileIO(char* image_name);
  void write_snapshot_header(FILE*, u_int32*);
  
  Oop initialInstanceOf(Oop);
  Oop nextInstanceAfter(Oop);
  
  Oop firstAccessibleObject();
  Oop nextObject(Oop obj);

  bool become_with_twoWay_copyHash(Oop, Oop, bool, bool);
  void do_all_oops_including_roots_here(Oop_Closure* oc, bool sync_with_roots);
  
  
  void save_to_checkpoint(FILE*);
  void restore_from_checkpoint(FILE*, int dataSize, int lastHash, int savedWindowSize, int fullScreenFlag);

  
  static inline int standard_partition() {
    return read_write;
  }
  
  Object* add_object_from_snapshot_to_a_local_heap_allocating_chunk(Oop dst_oop, Object* src_obj_wo_preheader) {
    Multicore_Object_Heap* h = local_heap_for_snapshot_object(src_obj_wo_preheader);
    Object* dst_obj = allocate_chunk_on_this_core_for_object_in_snapshot(h, src_obj_wo_preheader);
    if (check_many_assertions) assert(((u_int32)dst_obj & (sizeof(Oop) - 1)) == 0);
    h->add_object_from_snapshot(dst_oop, dst_obj, src_obj_wo_preheader);
    return dst_obj;
  }

  void initialize_helper();
};

# define FOR_ALL_HEAPS(rank, mutability) \
  FOR_ALL_RANKS(rank) \
    for (int mutability = 0;  mutability < max_num_mutabilities;  ++mutability)  \


