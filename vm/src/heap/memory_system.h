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
 * The heap is allocated per core and the memory of each core is divided
 * into a read-mostly and a read-write part.
 *
 * 
 * Conceptual Structure
 * --------------------
 *   TODO: add something about the object table
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
 * See Memory_System::map_read_write_and_read_mostly_memory.
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
class Memory_System {

private:
  char* image_name;

public:
  static const int max_num_mutabilities = 2;
  static const int read_mostly = 0;
  static const int read_write = 1;
  Multicore_Object_Heap* heaps[Max_Number_Of_Cores][max_num_mutabilities];

private:
  static const size_t normal_page_size =       PAGE_SIZE;
  static const size_t   huge_page_size = LARGE_PAGE_SIZE;

  // On the Tilera it might be necessary to specifiy the number of required
  // huge pages at boot time to use the use_huge_pages flag (hugepages=56)
public:
  static bool use_huge_pages;   // threadsafe readonly config value
  static size_t min_heap_MB;      // threadsafe readonly
  static bool replicate_methods;// threadsafe readonly
  static bool replicate_all;    // threadsafe readonly
  static bool OS_mmaps_up;      // threadsafe readonly

private:
  static u_int32 memory_per_read_write_heap; // threadsafe readonly, will always be power of two
  static u_int32 memory_per_read_mostly_heap; // threadsafe readonly, will always be power of two
  static u_int32 log_memory_per_read_write_heap;  // threadsafe readonly
  static u_int32 log_memory_per_read_mostly_heap; // threadsafe readonly

  char * read_write_memory_base,   * read_write_memory_past_end;
  char * read_mostly_memory_base,  * read_mostly_memory_past_end;

  struct global_GC_values {
    int32 growHeadroom;
    int32 shrinkThreshold;
    u_int32 gcCount, gcMilliseconds;
    u_int64 gcCycles;
    u_int32 mutator_start_time, last_gc_ms, inter_gc_ms;
  };
  struct global_GC_values* global_GC_values;

  int second_chance_cores_for_allocation[max_num_mutabilities];  // made threadsafe to increase the reliability of the value

  size_t page_size_used_in_heap;

  static int round_robin_period;




public:
  Memory_System();

  class Snapshot_Window_Size {
    int32 _fullScreenFlag;
    int32 _savedWindowSize;
  public:
    void initialize(int32 sws, int32 fsf) {
      _savedWindowSize = sws;
      _fullScreenFlag = fsf;
    }
    int32 fullScreenFlag() { return _fullScreenFlag; }
    int32 savedWindowSize() { return _savedWindowSize; }
    void fullScreenFlag(int32 fsf) { _fullScreenFlag = fsf; }
    void savedWindowSize(int32 sws) { _savedWindowSize = sws; }
  } snapshot_window_size;     // threadsafe readonly

  struct init_buf {
    int32 snapshot_bytes, sws, fsf, lastHash;
    char* read_mostly_memory_base;
    char* read_write_memory_base;
    u_int32 total_read_write_memory_size;
    u_int32 memory_per_read_write_heap;
    u_int32 log_memory_per_read_write_heap;
    u_int32 total_read_mostly_memory_size;
    u_int32 memory_per_read_mostly_heap;
    u_int32 log_memory_per_read_mostly_heap;
    int32 page_size;
    int32 main_pid;
    Object_Table* object_table;
    struct global_GC_values* global_GC_values;
  };


  Object_Table* object_table;   // threadsafe readonly



  void initialize_from_snapshot(int32 snapshot_bytes, int32 sws, int32 fsf, int32 lastHash);
  inline Object* allocate_chunk_on_this_core_for_object_in_snapshot(Multicore_Object_Heap*, Object*);

  void finished_adding_objects_from_snapshot();
  static void set_round_robin_period(int x) { round_robin_period = x; }


  void compute_snapshot_offsets(u_int32 *offsets);
  
 
  int32 adjust_for_snapshot(void* addr, u_int32* address_offsets) const {
    return (int32)addr - address_offsets[&heaps[rank_for_address(addr)][mutability_for_address(addr)] - &heaps[0][0]];
  }


private:
  void set_page_size_used_in_heap();
  int calculate_total_read_write_pages(int);
  int calculate_total_read_mostly_pages(int);
  int calculate_bytes_per_read_mostly_heap(int);
  bool ask_Linux_for_huge_pages(int);
  int how_many_huge_pages();
  void request_huge_pages(int);
  void  map_read_write_and_read_mostly_memory(int pid, size_t, size_t);
  void set_second_chance_cores_for_allocation(int);

public:

  void initialize_main(init_buf*);
  void initialize_helper();

  void create_my_heaps(init_buf*);
  void init_values_from_buffer(init_buf*);


  void ask_cpu_core_to_add_object_from_snapshot_allocating_chunk(Oop dst_oop, Object* src_obj_wo_preheader) {
    int rank = object_table->rank_for_adding_object_from_snapshot(dst_oop);
    Object_p dst_obj = (Object_p)The_Interactions.add_object_from_snapshot_allocating_chunk(rank, dst_oop, src_obj_wo_preheader);
    object_table->set_object_for(dst_oop, dst_obj  COMMA_FALSE_OR_NOTHING);
  }


  Object* add_object_from_snapshot_to_a_local_heap_allocating_chunk(Oop dst_oop, Object* src_obj_wo_preheader) {
    Multicore_Object_Heap* h = local_heap_for_snapshot_object(src_obj_wo_preheader);
    Object* dst_obj = allocate_chunk_on_this_core_for_object_in_snapshot(h, src_obj_wo_preheader);
    if (check_many_assertions) assert(((u_int32)dst_obj & (sizeof(Oop) - 1)) == 0);
    h->add_object_from_snapshot(dst_oop, dst_obj, src_obj_wo_preheader);
    return dst_obj;
  }

private:

  Multicore_Object_Heap* local_heap_for_snapshot_object(Object* src_obj_wo_preheader) {
    return heaps[Logical_Core::my_rank()][src_obj_wo_preheader->mutability_for_snapshot_object()];
  }

public:

  int round_robin_rank();
  int assign_rank_for_snapshot_object();

 
  bool contains(void* p) const {
    return read_mostly_memory_base <= (char*)p  &&  (char*)p < read_write_memory_past_end;
  }
  
  int mutability_for_address(void* p) const {
    // compiler bug:
    static const int c = read_write;
    static const int i = read_mostly;
    return is_address_read_write(p) ? c : i;
  }
  
  int rank_for_address(void* p) const {
    bool is_rw = is_address_read_write(p);
    u_int32 delta  = (char*)p - (is_rw ? read_write_memory_base : read_mostly_memory_base);
    u_int32 result = delta >> (is_rw ? log_memory_per_read_write_heap : log_memory_per_read_mostly_heap);
    assert(result ==  delta / (is_rw ? memory_per_read_write_heap : memory_per_read_mostly_heap));
    assert(result < (u_int32)Logical_Core::group_size);
    return (int)result;
  }

  Multicore_Object_Heap* heap_containing(void* obj) {
    return heaps[rank_for_address(obj)][mutability_for_address(obj)];
  }





  Object*  object_for_unchecked(Oop x) {
    if (Use_Object_Table) {
      Object* r = object_table->object_for(x);
      assert(object_table->probably_contains_not(r));
      return r;
    }
    else {
      return object_for(x);
    }
  }


  Object*  object_for(Oop x) {
    if (Use_Object_Table)
      return object_table->object_for(x);
    else
      return x.as_object();
  }



  Logical_Core* coreWithSufficientSpaceToAllocate(oop_int_t bytes, int);
  bool sufficientSpaceAfterGC(oop_int_t, int);

  void scan_compact_or_make_free_objects_everywhere(bool compacting, Abstract_Mark_Sweep_Collector*);
  void scan_compact_or_make_free_objects_here(bool compacting, Abstract_Mark_Sweep_Collector*);
  u_int32 bytesLeft(bool);
  u_int32 maxContiguousBytesLeft();

  void set_lowSpaceThreshold(int32);

  int32 get_lowSpaceThreshold()  {
    return heaps[Logical_Core::my_rank()][read_write]->get_lowSpaceThreshold();
  }

  void set_growHeadroom(int32 h) { global_GC_values->growHeadroom = h; }
  void set_shrinkThreshold(int32 s) { global_GC_values->shrinkThreshold = s; }
  int32 get_growHeadroom() { return global_GC_values->growHeadroom; }
  int32 get_shrinkThreshold() { return global_GC_values->shrinkThreshold; }

  void fullGC(const char*);
  void incrementalGC() {  if (check_assertions) lprintf("no incremental GC\n"); }
  void finalize_weak_arrays_since_we_dont_do_incrementalGC();

  bool become_with_twoWay_copyHash(Oop, Oop, bool, bool);
protected:
  void swapOTEs(Oop* o1, Oop* o2, int len);
  void level_out_heaps_if_needed();
public:

  Oop initialInstanceOf(Oop);
  Oop nextInstanceAfter(Oop);

  Oop firstAccessibleObject();
  Oop nextObject(Oop obj);


  void handle_low_space_signals();


  void  imageNamePut_on_this_core(const char*, int);
  void  imageNameGet(Object_p, int);
  int   imageNameSize();
  char* imageName();

  void flushExternalPrimitives();

  void snapshotCleanUp();
  void writeImageFile(char*);
private:
  void writeImageFileIO(char* image_name);
  void write_snapshot_header(FILE*, u_int32*);
  int32 max_lastHash();


public:

  void putLong(int32 x, FILE* f);

public:

  u_int32 bytesUsed();
  bool shuffle_or_spread(int, int, bool, bool, bool);
  bool shuffle_or_spread_last_part_of_a_heap(Object*, int, int, bool, bool, bool);
private:
  Multicore_Object_Heap* biggest_heap();
  int32 smallest_heap(int mutability);
public:
  bool moveAllToRead_MostlyHeaps();


  Oop get_stats(int);

  bool verify_if(bool);
  bool verify() { return verify_if(true); }

  void save_to_checkpoint(FILE*);
  void restore_from_checkpoint(FILE*, int dataSize, int lastHash, int savedWindowSize, int fullScreenFlag);

  // Cannot accept GC requests, defers to next BC boundary
  void enforce_coherence_before_store_into_object_by_interpreter(void* p, int nbytes, Object_p dst_obj_to_be_evacuated);

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

  void store_bytes_enforcing_coherence(void* dst, const void* src, int nbytes,   Object_p dst_obj_to_be_evacuated_or_null);

  void  pre_cohere_object_table(void* p, int sz) {  pre_cohere(p, sz); }
  void post_cohere_object_table(void* p, int sz) { post_cohere(p, sz); }

  void pre_cohere(void*, int);
  void post_cohere(void*, int);

  void enforce_coherence_before_this_core_stores_into_all_heaps();
  void enforce_coherence_after_this_core_has_stored_into_all_heaps();

  void enforce_coherence_before_each_core_stores_into_its_own_heap();
  void enforce_coherence_after_each_core_has_stored_into_its_own_heap();
  void invalidate_heaps_and_fence(bool);


  void print_heaps();
  void print();

  inline bool is_address_read_mostly(void* p) const {
    // don't use read_mostly_heap->contains(p) because that is slower, as it tests the bottom before the top
    // in the common case, p is in the read_write heap, which is above the read_mostly one.
    return (char*)p < read_mostly_memory_past_end;
  }
  inline bool is_address_read_write(void* p) const { return !is_address_read_mostly(p); }

  void do_all_oops_including_roots_here(Oop_Closure* oc, bool sync_with_roots);
  
private:
  static char  mmap_filename[BUFSIZ];
  static char* map_heap_memory(size_t total_size, size_t bytes_to_map,
                               void* where, off_t offset,
                               int main_pid, int flags);  
};

