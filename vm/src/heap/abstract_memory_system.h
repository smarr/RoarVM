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


class Abstract_Memory_System {
protected:
  static const size_t normal_page_size =       PAGE_SIZE;
  static const size_t   huge_page_size = LARGE_PAGE_SIZE;
  
  char* image_name;

 
  // On the Tilera it might be necessary to specifiy the number of required
  // huge pages at boot time to use the use_huge_pages flag (hugepages=56)
public:
  static bool use_huge_pages;   // threadsafe readonly config value
  static size_t min_heap_MB;      // threadsafe readonly
  static bool replicate_methods;// threadsafe readonly
  static bool replicate_all;    // threadsafe readonly
  static bool OS_mmaps_up;      // threadsafe readonly

  Abstract_Memory_System();
  
  void  imageNamePut_on_this_core(const char*, int);
  int   imageNameSize();
  char* imageName();

# warning STEFAN: I think, Snapshot_Window_Size belongs not in here. TODO: refactor
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


  int round_robin_rank();
  int assign_rank_for_snapshot_object();
  
  Multicore_Object_Table* object_table;   // threadsafe readonly
  
  void ask_cpu_core_to_add_object_from_snapshot_allocating_chunk(Oop dst_oop, Object* src_obj_wo_preheader) {
    int rank = object_table->rank_for_adding_object_from_snapshot(dst_oop);
    Object_p dst_obj = (Object_p)The_Interactions.add_object_from_snapshot_allocating_chunk(rank, dst_oop, src_obj_wo_preheader);
    object_table->set_object_for(dst_oop, dst_obj  COMMA_FALSE_OR_NOTHING);
  }
  
  Object*  object_for_unchecked(Oop x) {
    Object* r = object_table->object_for(x);
    assert(!object_table->probably_contains(r));
    return r;
  }
  
  Object*  object_for(Oop x) {
    return object_table->object_for(x);
  }
public:
  
  void putLong(int32 x, FILE* f);

  
  
protected:
  static u_int32 memory_per_read_write_heap; // threadsafe readonly, will always be power of two
  static u_int32 log_memory_per_read_write_heap;  // threadsafe readonly
  
  char * read_write_memory_base,   * read_write_memory_past_end;
  
  
  struct Global_GC_Values {
    int32 growHeadroom;
    int32 shrinkThreshold;
    u_int32 gcCount, gcMilliseconds;
    u_int64 gcCycles;
    u_int32 mutator_start_time, last_gc_ms, inter_gc_ms;
  };
  struct Global_GC_Values* global_GC_values;
  
  size_t page_size_used_in_heap;
  
  static int round_robin_period;
  
  
  int calculate_pages_for_segmented_heap(int page_size);
  
  void swapOTEs(Oop* o1, Oop* o2, int len);

  
public:
  void set_growHeadroom(int32 h)    { global_GC_values->growHeadroom = h; }
  void set_shrinkThreshold(int32 s) { global_GC_values->shrinkThreshold = s; }
  int32 get_growHeadroom()          { return global_GC_values->growHeadroom; }
  int32 get_shrinkThreshold()       { return global_GC_values->shrinkThreshold; }

  
  void writeImageFile(char*);
  
  static void set_round_robin_period(int x) { round_robin_period = x; }

    
};


