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
 * The Memory_System manages the heap structure that is eventually used for storing Smalltalk
 * objects.
 *
 *
 * > The heap structure is *conceptually* divided into pages of page_size bytes, which are
 *   managed by the Memory System. 
 *
 *   Every Core's Object Heap will lazily request pages with the Memory System to allocate 
 *   Chunks of memory (i.e., for storing Smalltalk objects). An Object Heap will fill its 
 *   requested page(s) until the remaining space is insufficient for allocating a new Chunk. 
 *   When this happens, the Object Heap will simply request one or more contiguous new pages 
 *   to allocate more Chunks.
 *   (cfr. Abstract_Object_Heap::allocateChunk(bytes), sufficientSpaceToAllocate(bytes) and
 *         Memory_System::allocate(bytes)).
 *
 *
 * > The parallel GC will protect the conceptual pages when their contents have to be moved (cfr. Parallel_GC_Thread; using mprotect).
 *   Because this protection can only be set per *actual* page (i.e., a page from the TLB), 
 *   all *conceptual pages* are actual-page-aligned (i.e., page_size is a multiple of 
 *   page_size_used_in_heap, which is the size of an actual page).
 *
 *
 * > All free pages are kept in a free list, in which they are sorted by ascending address.
 *   As a consequence, freeing a page is an O(n) operation, but it's easy to allocate
 *   contiguous pages. Allocating one or more contiguous pages to Object Heaps is also
 *   an O(n) operation, but allocating a single page can be done in constant time.
 *
 *
 * > The heap structure's visual representation:
 *      
 *       +----------+----------+----------+-----------+----------+
 *       |    P1    |    P2    |    P3    |    ...    |    Pn    |
 *       +----------+----------+----------+-----------+----------+
 *       ↑          ↑                                            ↑
 *       0      page_size                                    n*page_size = n*m*page_size_used_in_heap
 *                                                            
 *
 *
 * > The free list is encoded in the heap structure, for instance:
 *
 *                     +----------+----------+----------+-----------+----------+
 *       free_page---->|    P1    |    P2    |    P3    |     P4    |    P5    |
 *                     +----+-----+-----+----+----------+------+----+----------+    NULL
 *                          |_____↑     |_______________↑      |______________________↑
 *
 *
 * > The heap is stored in a single, physical memory space, but this physical memory space
 *   is enitrely mapped from *two* virtual memory spaces (using mmap):
 *
 *     1. [heap_base, heap_past_end[: 
 *          The typically used virtual memory space. Some of its conceptual pages can be protected
 *          during a GC cycle (i.e., when their objects have to be relocated).
 *          (see Parallel_GC_Thread.h/cpp for more information)
 *
 *     2. [heap_base + unprotected_heap_offset, heap_past_end + unprotected_heap_offset[:
 *          The virtual memory space used for relocating objects. The conceptual pages in this
 *          virtual memory space are *never* protected and hence this extra virtual memory space 
 *          allows to copy out the contents from protected pages to their new location.
 *  
 *     The memory mapping is done on the main core and all other cores receive the base virtual addresses 
 *     via a message to map the same memory regions into the same addresses.
 *
 *     For thread-based systems, this is only done on the main core, and the other cores do not need to 
 *     map in any memory.
 *
 *     A temporary file is used to ensure that all cores are working on the same
 *     memory. This file actually provides the single, physical memory space.
 *
 *  > Every free page is represented as a Free Object (i.e., it has the free object header). In addition,
 *    a non-free page has a Free Object at its end (i.e., after the last allocated chunk, if there's
 *    enough space left). As such, it's easy to iterate over the contents of the heap.
 *    (cfr. FOR_EACH_OBJECT)
 *
 *
 *  > To support the parallel GC, the Memory System also keeps track of which conceptual pages have been 
 *    allocated (i.e., are not on the free list). To this end, it keeps a liveness array that has an LPage 
 *    entry (cfr. page.h) for every conceptual page. A copy of this array is used during every GC cycle. 
 *    
 *    See Parallel_GC_Thread.h/cpp for more information.
 *
 */
 

/** prototypes **/
struct Page;
struct LPage;
/****************/

static const int page_size = 1 * Mega; // the conceptual pages' size (necessarily multiple of page_size_used_in_heap!)

class Memory_System {

public:
  /* Global heap */
  char * heap_base, * heap_past_end; 
  size_t page_size_used_in_heap;      // the actual pages' size (i.e., of the pages in the TLB)
  int    unprotected_heap_offset;     // cfr. unprotected_heap_base
  
  /* Core-local heaps: each one can be backed by a piece of page-aligned memory from the global heap */
  Multicore_Object_Heap* heaps[Max_Number_Of_Cores];
  
  /* Page Management */
  Page * allocate(size_t);
  void   free(Page *);
  int    freePages();
  Page * firstPage();
  
  /* GC Liveness Support */
  void    startGCpreparation();
  LPage * stopGCpreparation();
  void    adjustLivenessCopyForCore(int, bool);
  
  char * heap_past_used_end();
  

private:
  char * unprotected_heap_base;     // Maps to the same physical memory as [heap_base,heap_past_end[, but will never
                                    // be protected and is meant for copying out contents of objects from protected pages.
                                    
  
  void pushPage(Page*);

  char* image_name;

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
  struct global_GC_values {
    int32 growHeadroom;
    int32 shrinkThreshold;
    u_int32 gcCount, gcMilliseconds;
    u_int64 gcCycles;
    u_int32 mutator_start_time, last_gc_ms, inter_gc_ms;
    
    // Extra globals
    OS_Interface::Mutex* mutex;
    char *               free_page;
    LPage *              liveness;                  // For 
    LPage *              livenessCopy;              // GC
    bool *               adjustLivenessCopyForCore; // support
  };
  struct global_GC_values* global_GC_values;

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
    char* heap_base;
    char* unprotected_heap_base;
    int32 page_size;
    int32 main_pid;
    Object_Table* object_table;
    struct global_GC_values* global_GC_values;
  };


  Object_Table* object_table;   // threadsafe readonly



  void initialize_from_snapshot(int32 snapshot_bytes, int32 sws, int32 fsf, int32 lastHash);
  void initialize_memory();
  inline Object* allocate_chunk_on_this_core_for_object_in_snapshot(Multicore_Object_Heap*, Object*);

  void finished_adding_objects_from_snapshot();
  static void set_round_robin_period(int x) { round_robin_period = x; }


  void compute_snapshot_offsets(u_int32 *offsets);

public:
  int calculate_total_pages(int);
private:
  void set_page_size_used_in_heap();
  
  bool ask_Linux_for_huge_pages(int);
  int how_many_huge_pages();
  void request_huge_pages(int);
  void map_memory(int pid, size_t);
  void verify_memory_initialization();

public:

  void initialize_main(init_buf*);
  void initialize_helper();

  void create_my_heap(init_buf*);
  void create_heap_for_GC();
  void init_values_from_buffer(init_buf*);
  
  Abstract_Object_Heap* my_heap(){
    return heaps[Logical_Core::my_rank()];
  }


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

  Multicore_Object_Heap* local_heap_for_snapshot_object(Object* /* src_obj_wo_preheader */) {
    return heaps[Logical_Core::my_rank()];
  }

public:
  int round_robin_rank();
  int assign_rank_for_snapshot_object();

  bool contains(void* p) const {
    return regular_virtual_space_contains((char*)p);                        // regular virtual address space
  }
  
  bool regular_virtual_space_contains(char* p) const {
    return (heap_base <= p  &&  p < heap_past_end);
  }
  
  bool unprotected_virtual_space_contains(void* p) const {
    return ((heap_base + unprotected_heap_offset) <= (char*)p  &&  (char*)p < (heap_past_end + unprotected_heap_offset));
  }


  Object*  object_for_unchecked(Oop x) {
    return object_for(x);
  }


  Object*  object_for(Oop x) {
    if (Use_Object_Table)
      return object_table->object_for(x);
    else
      return x.as_object();
  }

  Logical_Core* coreWithSufficientSpaceToAllocate(oop_int_t bytes);
  bool sufficientSpaceToAllocate(oop_int_t);
  bool sufficientSpaceAfterGC(oop_int_t);

  void scan_compact_or_make_free_objects_everywhere(bool compacting, Abstract_Mark_Sweep_Collector*);
  void scan_compact_or_make_free_objects_here(bool compacting, Abstract_Mark_Sweep_Collector*);
  u_int32 bytesTotal();
  u_int32 bytesLeft(bool);
  u_int32 bytesLeftIncludingPages();
  u_int32 maxContiguousBytesLeft();

  void set_lowSpaceThreshold(int32);

  int32 get_lowSpaceThreshold()  {
    return heaps[Logical_Core::my_rank()]->get_lowSpaceThreshold();
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
  void write_snapshot_header(FILE*, char*);
  void write_snapshot_heap(FILE*, char*);
  int32 max_lastHash();


public:

  void putLong(int32 x, FILE* f);

public:

  u_int32 bytesUsed();
private:
  Multicore_Object_Heap* biggest_heap();
  int32 smallest_heap();
public:


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
    pre_cohere(p, nbytes);
  }
  
  void enforce_coherence_after_store_into_object_by_interpreter(void* /* p */, int /* nbytes */) {}
  
  void enforce_coherence_after_store(void* p, int nbytes)  {
    assert(contains(p));
    post_cohere(p, nbytes);
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

  void do_all_oops_including_roots_here(Oop_Closure* oc, bool sync_with_roots);
  
private:
  static char  mmap_filename[BUFSIZ];
  static char* map_heap_memory(size_t total_size, size_t bytes_to_map,
                               void* where, off_t offset,
                               int main_pid, int flags);  
};

# define FOR_EACH_OBJECT(object_ptr) \
for ( Object* object_ptr  =  ((Chunk*)(The_Memory_System()->heap_base))->object_from_chunk(); \
              object_ptr !=  NULL; \
              object_ptr  =  object_ptr->nextObject() )

# define FOR_EACH_OBJECT_EFFICIENT(object_ptr, past_used_end) \
char* past_used_end = The_Memory_System()->heap_past_used_end(); \
for ( Object* object_ptr  =  ((Chunk*)(The_Memory_System()->heap_base))->object_from_chunk(); \
object_ptr !=  NULL && object_ptr < (Object*)past_used_end; \
object_ptr  =  object_ptr->nextObject() )
