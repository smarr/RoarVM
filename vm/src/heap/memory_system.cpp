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


#include "headers.h"

bool     Memory_System::use_huge_pages = On_Tilera;
bool     Memory_System::replicate_methods = false; // if true methods are put on read-mostly heap
bool     Memory_System::replicate_all = true; // if true, all (non-contexts) are allowed in read-mostly heap
bool     Memory_System::OS_mmaps_up = On_Apple;
  int    Memory_System::round_robin_period = 1;
  size_t Memory_System::min_heap_MB =  On_iOS ? 32 : On_Tilera ? 256 : 1024; // Fewer GCs on Mac


Memory_System::Memory_System() {
  image_name = new char[1]; *image_name = '\0';

  global_GC_values = (struct global_GC_values*)Memory_Semantics::shared_malloc(sizeof(struct global_GC_values));
  global_GC_values->growHeadroom = 4 * Mega;
  global_GC_values->shrinkThreshold = 8 * Mega;
  global_GC_values->gcCycles = 0;
  global_GC_values->gcCount = 0;
  global_GC_values->gcMilliseconds = 0;
  global_GC_values->mutator_start_time = 0;
  global_GC_values->last_gc_ms = 0;
  global_GC_values->inter_gc_ms = 0;
  global_GC_values->free_page = NULL;
  global_GC_values->mutex = (OS_Interface::Mutex*)Memory_Semantics::shared_malloc(sizeof(OS_Interface::Mutex));
  OS_Interface::mutex_init(global_GC_values->mutex);
  
  page_size_used_in_heap = 0;

  for (int rank = 0;  rank < Max_Number_Of_Cores;  ++rank) {
      heaps[rank] = NULL;
  }
}


void Memory_System::finished_adding_objects_from_snapshot() {
  object_table->post_store_whole_enchillada();
  The_Squeak_Interpreter()->set_am_receiving_objects_from_snapshot(false);
  enforce_coherence_after_each_core_has_stored_into_its_own_heap();
  
  // now all objects are in the heap, so we are also sure that this file is
  // in memory and the filesystem link is not to be used by mmap anymore
  unlink(mmap_filename);
}

void Memory_System::enforce_coherence_after_each_core_has_stored_into_its_own_heap() {
  if (!replicate_methods &&  !replicate_all) return;  // should not need this statement, but there was a bug without it, and anyway it's faster with it
  OS_Interface::mem_fence(); // ensure all cores see same heap _next's
  
  enforceCoherenceAfterEachCoreHasStoredIntoItsOwnHeapMessage_class().send_to_other_cores();
  
  heaps[Logical_Core::my_rank()]->enforce_coherence_in_whole_heap_after_store();
}

void Memory_System::enforce_coherence_before_each_core_stores_into_its_own_heap() {
  if (!replicate_methods &&  !replicate_all) return; // should not need this statement, but there was a bug without it, and anyway it's faster with it
  enforceCoherenceBeforeEachCoreStoresIntoItsOwnHeapMessage_class().send_to_other_cores();
  invalidate_heaps_and_fence(false);
}


void Memory_System::enforce_coherence_before_this_core_stores_into_all_heaps() {
  if (!replicate_methods &&  !replicate_all) return; // should not need this statement, but there was a bug without it, and anyway it's faster with it
  OS_Interface::mem_fence(); // ensure all cores see same heap _next's
  enforceCoherenceBeforeSenderStoresIntoAllHeapsMessage_class().send_to_other_cores();
}



bool Memory_System::verify_if(bool condition) {
  if (!condition)
    return true;
  
  bool ok = true;
  Object *prev_obj = NULL;
  __attribute__((unused)) Object *prev_prev_obj = NULL; // debugging
  FOR_EACH_OBJECT(obj) {
    if (obj->is_marked()) {
      lprintf("object 0x%x should not be marked but is; header is 0x%x\n",
      obj, obj->baseHeader);
      fatal("");
    }
    if (!obj->isFreeObject() &&  obj->is_current_copy())
      ok = obj->verify() && ok;
    
    if (!ok) dittoing_stdout_printer->printf("Failed to verify obj at %p\n", obj);
    
    prev_prev_obj = prev_obj;
    prev_obj = obj;
  }
  dittoing_stdout_printer->printf("Object_Heap %sverified\n", ok ? "" : "NOT ");
  
  return ok;
}


Oop Memory_System::get_stats(int what_to_sample) {
  int s = The_Squeak_Interpreter()->makeArrayStart();
  if (what_to_sample & (1 << SampleValues::gcStats)) {
    PUSH_POSITIVE_32_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(global_GC_values->gcCount);
    PUSH_POSITIVE_32_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(global_GC_values->gcMilliseconds);
    PUSH_POSITIVE_64_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(global_GC_values->gcCycles);
    global_GC_values->gcCycles = global_GC_values->gcCount = global_GC_values->gcMilliseconds = 0;
  }
  if (what_to_sample & (1 << SampleValues::heapStats)) {
    Oop readWriteHeapStats = The_Memory_System()->heaps[Logical_Core::my_rank()]->get_stats();
    PUSH_WITH_STRING_FOR_MAKE_ARRAY(readWriteHeapStats);
    Oop readMostlyHeapStats = The_Memory_System()->heaps[Logical_Core::my_rank()]->get_stats();
    PUSH_WITH_STRING_FOR_MAKE_ARRAY(readMostlyHeapStats);
  }

  return The_Squeak_Interpreter()->makeArray(s);
}


void Memory_System::fullGC(const char* why) {
  lprintf("fullGC: *TODO*: probably requires changes.\n");
  return;
  
# if Use_Object_Table
  Squeak_Interpreter * const interp = The_Squeak_Interpreter();
  if (interp->am_receiving_objects_from_snapshot())
    fatal("cannot gc now");
  
  PERF_CNT(interp, count_full_gc());

  lprintf("about to fullGC: %s\n", why);
  global_GC_values->inter_gc_ms = global_GC_values->mutator_start_time ? interp->ioWhicheverMSecs() - global_GC_values->mutator_start_time : 0;
  u_int32 last_gc_start = interp->ioWhicheverMSecs();
  
  global_GC_values->gcCycles -= OS_Interface::get_cycle_count();
  
  Mark_Sweep_Collector msc;
  msc.gc();
  
  ++global_GC_values->gcCount;
  global_GC_values->gcMilliseconds += (global_GC_values->last_gc_ms = interp->ioWhicheverMSecs() - last_gc_start);
  global_GC_values->gcCycles += OS_Interface::get_cycle_count();
  
  global_GC_values->mutator_start_time = interp->ioWhicheverMSecs();

  level_out_heaps_if_needed();
# else
  # warning STEFAN: The GC is currently disabled, since it is not adapted to \
                    work without object table.
# endif
}


void Memory_System::finalize_weak_arrays_since_we_dont_do_incrementalGC() {
  fullGC("finalize_weak_arrays_since_we_dont_do_incrementalGC");
}


void Memory_System::swapOTEs(Oop* o1, Oop* o2, int len) {
# if Use_Object_Table
  for (int i = 0;  i < len;  ++i) {
    Object_p obj1 = o1[i].as_object();
    Object_p obj2 = o2[i].as_object();

    obj2->set_object_address_and_backpointer(o1[i]  COMMA_TRUE_OR_NOTHING);
    obj1->set_object_address_and_backpointer(o2[i]  COMMA_TRUE_OR_NOTHING);
  }
# else
  fatal("Currently not supported without an object_table.");
# endif
}


static bool containOnlyOops(Object_p a1, Object_p a2) {
  for (u_oop_int_t fieldOffset = a1->lastPointer() / sizeof(Oop);
       fieldOffset >= Object::BaseHeaderSize / sizeof(Oop);
       --fieldOffset )
    if (   !a1->as_oop_p()[fieldOffset].is_mem()
        || !a2->as_oop_p()[fieldOffset].is_mem() )
      return false;
  return true;
}





class Abstract_Become_Closure: public Oop_Closure {
protected:
  Object_p array1;
  Object_p array2;
  Oop *o1, *o2;
  int len;
  bool twoWay, copyHash;

  Abstract_Become_Closure(Object_p a1, Object_p a2, bool t, bool c) : Oop_Closure() {
    array1 = a1;
    array2 = a2;
    o1 = array1->as_oop_p() + Object::BaseHeaderSize/sizeof(Oop);
    o2 = array2->as_oop_p() + Object::BaseHeaderSize/sizeof(Oop);
    len = (a1->lastPointer() - Object::BaseHeaderSize) / sizeof(Oop)  +  1;
    twoWay = t;  copyHash = c;
  }
public:
  void copyHashes() {
    if (!copyHash) return;
    for (int i = 0;  i < len;  ++i) {
      Object_p obj1 = o1[i].as_object();  oop_int_t* hdr1p = &obj1->baseHeader;  oop_int_t hdr1 = *hdr1p;
      Object_p obj2 = o2[i].as_object();  oop_int_t* hdr2p = &obj2->baseHeader;  oop_int_t hdr2 = *hdr2p;
      if (twoWay) {
        The_Memory_System()->store_enforcing_coherence(hdr1p, (hdr1 & ~Object::HashMask)  |  (hdr2 & Object::HashMask), obj1);
      }
      The_Memory_System()->store_enforcing_coherence(hdr2p, (hdr2 & ~Object::HashMask)  |  (hdr1 & Object::HashMask), obj2);
    }
  }

  virtual const char* class_name() { return "Abstract_Become_Closure"; }
};

class One_Way_Become_Closure: public Abstract_Become_Closure {
public:
  One_Way_Become_Closure(Object_p a1, Object_p a2, bool c) : Abstract_Become_Closure(a1, a2, false, c) {  }

  void value(Oop* p, Object_p containing_obj_or_null) {
    if (containing_obj_or_null != array1)
      for (int i = 0;  i < len;  ++i)
        if (*p == o1[i]) {
          The_Memory_System()->store_enforcing_coherence_if_in_heap(p, o2[i], containing_obj_or_null);
        }
  }
  virtual const char* class_name() { return "One_Way_Become_Closure"; }
};

class Two_Way_Become_Closure: public Abstract_Become_Closure {
public:
  Two_Way_Become_Closure(Object_p a1, Object_p a2, bool c) : Abstract_Become_Closure(a1, a2, true, c) {}


  void value(Oop* p, Object_p containing_obj_or_null) {
    if (containing_obj_or_null != array1  &&  containing_obj_or_null != array2)
      for (int i = 0;  i < len;  ++i) {
        Oop x = *p;

             if (x == o1[i]) { The_Memory_System()->store_enforcing_coherence_if_in_heap(p, o2[i], containing_obj_or_null); }
        else if (x == o2[i]) { The_Memory_System()->store_enforcing_coherence_if_in_heap(p, o1[i], containing_obj_or_null); }
      }
  }
  virtual const char* class_name() { return "Two_Way_Become_Closure"; }
};




bool Memory_System::become_with_twoWay_copyHash(Oop array1, Oop array2, bool twoWayFlag, bool copyHashFlag) {
  Safepoint_for_moving_objects sf("become");
  Safepoint_Ability sa(false);

  if (!array1.isArray()  ||  !array2.isArray())  return false;
  Object_p a1o = array1.as_object();
  Object_p a2o = array2.as_object();
  if (a1o->lastPointer() != a2o->lastPointer()) return false;
  if (!containOnlyOops(a1o, a2o)) return false;
  
  Oop classP = The_Squeak_Interpreter()->splObj(Special_Indices::ClassProcess);
  for (int i = 0, n = a1o->fetchWordLength(); i < n; ++i)
    if (a1o->fetchPointer(i) == classP  ||  a2o->fetchPointer(i) == classP)
      The_Squeak_Interpreter()->set_process_object_layout_timestamp(The_Squeak_Interpreter()->process_object_layout_timestamp() + 1);


  // sync?
  if (twoWayFlag  &&  copyHashFlag) {
    swapOTEs(a1o->as_oop_p() + Object::BaseHeaderSize/sizeof(Oop),
             a2o->as_oop_p() + Object::BaseHeaderSize/sizeof(Oop),
             (a1o->lastPointer() - Object::BaseHeaderSize) / sizeof(Oop)  +  1);
    return true;
  }

  if (twoWayFlag) {
    Two_Way_Become_Closure bc(a1o, a2o, copyHashFlag);
    do_all_oops_including_roots_here(&bc, true); // will not do the contents of the arrays themselves
  }
  else {
    One_Way_Become_Closure bc(a1o, a2o, copyHashFlag);
    do_all_oops_including_roots_here(&bc, true); // will not do the contents of the arrays themselves
  }
  flushInterpreterCachesMessage_class().send_to_all_cores();
  return true;
}

Logical_Core* Memory_System::coreWithSufficientSpaceToAllocate(oop_int_t bytes) {
  Multicore_Object_Heap* h = heaps[Logical_Core::my_rank()];
  int minFree = bytes + 10000 + h->lowSpaceThreshold; // may not be necessary
  if  ( h->sufficientSpaceToAllocate(minFree) || sufficientSpaceToAllocate(minFree))
    return Logical_Core::my_core();
  else
    return NULL;
}

bool Memory_System::sufficientSpaceToAllocate(oop_int_t minFree) {
  return freePages() * page_size >= minFree;
}

bool Memory_System::sufficientSpaceAfterGC(oop_int_t minFree) {
  fullGC("sufficientSpaceAfterGC");

  if (The_Squeak_Interpreter()->signalLowSpace())
    return false;  
  
  return freePages() * page_size >= minFree;
}



void Memory_System::imageNamePut_on_this_core(const char* n, int len) {
  delete[] image_name;
  image_name = new char[len + 1];
  bcopy(n, image_name, len);
  image_name[len] = '\0';
}


void Memory_System::imageNameGet(Object_p dst, int len) {
  char* n = dst->as_char_p() + Object::BaseHeaderSize;
  assert(The_Memory_System()->contains(n));

  enforce_coherence_before_store_into_object_by_interpreter(n, len, dst);
  strncpy(n, image_name, len);
  enforce_coherence_after_store_into_object_by_interpreter(n, len);
}
int Memory_System::imageNameSize() { return strlen(image_name); }

char* Memory_System::imageName() { return image_name; }

void Memory_System::flushExternalPrimitives() {
  FOR_EACH_OBJECT(oop) {
      if (!oop->isFreeObject()
        &&   oop->isCompiledMethod()
        &&   oop->primitiveIndex() == Squeak_Interpreter::PrimitiveExternalCallIndex)
      oop->flushExternalPrimitive();
  }
}

void Memory_System::handle_low_space_signals() {
  // fatal("*TODO* is this still relevant?");
  
  FOR_ALL_RANKS(rank) {
    heaps[rank]->handle_low_space_signal();
  }
}


Oop Memory_System::initialInstanceOf(Oop x) {
  FOR_EACH_OBJECT(obj)
    if (!obj->isFreeObject()  &&  obj->fetchClass() == x )
      return obj->as_oop();

  return The_Squeak_Interpreter()->roots.nilObj;
}


Oop Memory_System::nextInstanceAfter(Oop x) {
  return x.as_object()->nextInstance();
}


void Memory_System::snapshotCleanUp() {
  FOR_EACH_OBJECT(obj) {
    if (obj->isFreeObject())
      continue;
    if (Object::Format::might_be_context(obj->format()) && obj->hasContextHeader()) {
      int bytes_to_last_pointer = obj->lastPointer();
      int bytes_to_pointer_after_last = bytes_to_last_pointer + sizeof(Oop);
      int total_bytes = obj->sizeBits();
      int oops_to_zap = (total_bytes - bytes_to_pointer_after_last) >> ShiftForWord;
      Oop* zap_start = obj->as_oop_p() + (bytes_to_pointer_after_last >> ShiftForWord);
      assert(The_Memory_System()->contains(zap_start));
      oopset_no_store_check(
                            zap_start,
                            The_Squeak_Interpreter()->roots.nilObj,
                            oops_to_zap);
    }
    else if (Object::Format::isCompiledMethod(obj->format())
             &&  obj->primitiveIndex() == Squeak_Interpreter::PrimitiveExternalCallIndex)
      obj->flushExternalPrimitive();
  }
}


u_int32 Memory_System::bytesTotal() {
  return heap_past_end - heap_base;
}

/* note: only on the object heaps' current pages */ 
u_int32 Memory_System::bytesLeft(bool includeSwap) {
  u_int32 sum = 0;
  FOR_ALL_RANKS(i)
    sum += heaps[i]->bytesLeft(includeSwap);
  return sum;
}

u_int32 Memory_System::bytesLeftIncludingPages() {
  u_int32 freePagesBytes = freePages() * page_size;
  
  return freePagesBytes + bytesLeft(false);
}


u_int32 Memory_System::bytesUsed() {
  return bytesTotal() - bytesLeftIncludingPages(); 
}


void Memory_System::writeImageFile(char* image_name) {
  writeImageFileIO(image_name);
  fn_t setMacType = The_Interactions.load_function_from_plugin(Logical_Core::main_rank, "setMacFileTypeAndCreator", "FilePlugin");
  if (setMacType != NULL)  (*setMacType)(The_Memory_System()->imageName(), "STim", "FAST");
}




int32 Memory_System::max_lastHash() {
  int r = 0;
  FOR_ALL_RANKS(rank) {
    r = max(r, heaps[rank]->get_lastHash());
  }
  return r;
}


static const int32 headerSize = 64;




void Memory_System::writeImageFileIO(char* image_name) {
  // int32 headerStart = 0;
  FILE* f = fopen(image_name, "wb");
  if (f == NULL) {
    perror("could not open file for writing");
    The_Squeak_Interpreter()->success(false);
    return;
  }
  
  char* end_of_used_heap = heap_past_used_end();

  write_snapshot_header(f, end_of_used_heap);

  if (!The_Squeak_Interpreter()->successFlag) {
    fclose(f);
    return;
  }

  if (fseek(f, headerSize, SEEK_SET)) {
    perror("seek");
    The_Squeak_Interpreter()->success(false);
    return;
  }
  
  write_snapshot_heap(f, end_of_used_heap); 
  
  fclose(f);
  return;
}

void Memory_System::write_snapshot_header(FILE* f, char* end_of_used_heap) {
  putLong(The_Squeak_Interpreter()->image_version, f);
  putLong(headerSize, f);
  putLong((end_of_used_heap - heap_base) - preheader_byte_size /* Squeak 64-bit VM bug workaround */, f); // dataSize
  // For explanation of preheader_byte_size above and below, see long comment about Squeak compatibility in write_image_file -- dmu 6/10
  putLong((int32)heap_base + preheader_byte_size/* Squeak 64-bit VM bug workaround */, f); // start of memory;
  putLong((int32)The_Squeak_Interpreter()->roots.specialObjectsOop.as_object(), f);
  putLong(max_lastHash(), f);
  int screenSize, fullScreenFlag;
  The_Interactions.get_screen_info(&screenSize, &fullScreenFlag);
  putLong(screenSize, f);
  putLong(fullScreenFlag, f);
  int32 extraVMMemory = 0;
  putLong(extraVMMemory, f);
  for (int i = 1;  i <= 7;  ++i)
    putLong(0, f);
}

void Memory_System::write_snapshot_heap(FILE* f, char* end_of_used_heap) {
  int     byteCount = 0; //debug
  bool    is_first_object = true;
  Object* last_obj = NULL;
  
  const oop_int_t preheader_placeholder = Object::make_free_object_header(preheader_byte_size);  
    
  FOR_EACH_OBJECT(obj) {
    if ((char*)obj >= end_of_used_heap) {
      /* past end of "active" heap */
      assert_always(obj->isFreeObject());
      continue;
    }
  
    /* Free objects (can have a preheader!) */
    if (obj->isFreeObject()) {
      assert_always_msg(!is_first_object, "first object must not be free to work with Squeak 64 bit VM"); // Squeak 64-bit VM bug workaround
      
      if (preheader_oop_size) {
        The_Memory_System()->putLong(preheader_placeholder, f); 
        byteCount += 4; 
      }
      
      int bytes = obj->sizeOfFree();
      oop_int_t* p = obj->as_oop_int_p();
      for (int i = 0;  i < bytes;  i += sizeof(int32)) {
        The_Memory_System()->putLong(*p++, f);
        byteCount += 4;
      }
      
      last_obj = obj;
      continue;
    }    
    
    /* Non-free objects */
    // Preheader (replace by a free object)
    assert(sizeof(long) == sizeof(Oop));
    if (preheader_oop_size  &&  !is_first_object /* see long comment above */) { // Squeak 64-bit VM bug workaround
      The_Memory_System()->putLong(preheader_placeholder, f);
      byteCount += 4;
      for (int i = 1;  i  <  preheader_oop_size;  ++i) {
        The_Memory_System()->putLong(Oop::Illegals::free_extra_preheader_words, f);
        byteCount += 4;
      }
    }
    
    // Extra Headers
    if (obj->contains_sizeHeader()) {
      The_Memory_System()->putLong(obj->sizeHeader(), f);
      byteCount += 4;
    }
 
    if (obj->contains_class_and_type_word()) {
      The_Memory_System()->putLong(
              Header_Type::extract_from( obj->class_and_type_word() )
              |  Header_Type::without_type( (int32)(obj->get_class_oop().as_object()) ),
              f);
      byteCount += 4;
    }
              
    // Base Header
    The_Memory_System()->putLong(obj->baseHeader, f);    
    byteCount += 4;
    
    // And the contents...
    oop_int_t* p;
    for ( p = &obj->baseHeader + 1;
         (Oop*)p <= obj->last_pointer_addr();
         ++p ) {
       Oop oop = *(Oop*)p;
       The_Memory_System()->putLong( oop.is_int()
                                      ? oop.bits()
                                      : (int32)oop.as_object()
                                    ,  f);
       byteCount += 4;
    }
    for (Chunk* next = obj->nextChunk();  p < (oop_int_t*)next;  The_Memory_System()->putLong(*p++, f))
      byteCount += 4; // bytes
      
    last_obj = obj;
    
    is_first_object = false;    
  }
  if (Verbose_Debug_Prints)
    lprintf("Written %d bytes from the heap to the image while expecting to write %d bytes.\n", 
            byteCount, (heap_past_used_end() - heap_base - preheader_byte_size));
}

Oop Memory_System::firstAccessibleObject() {
  FOR_EACH_OBJECT(obj) {
    if(!obj->isFreeObject())
      return obj->as_oop();
  }
  return The_Squeak_Interpreter()->roots.nilObj;
}


Oop Memory_System::nextObject(Oop x) {
  return x.as_object()->nextAccessibleObject()->as_oop();
}


void  Memory_System::set_lowSpaceThreshold(int32 x)  {
  FOR_ALL_RANKS(rank)
    heaps[rank]->set_lowSpaceThreshold(x);
}

int Memory_System::round_robin_rank() {
  assert(Logical_Core::running_on_main());
  static int i = 0; // threadsafe? think its ok, there is no need for 100% monotony, Stefan, 2009-09-05
  return i++ % Logical_Core::group_size;
}


int Memory_System::calculate_total_pages(int page_size) {
  int min_heap_bytes = min_heap_MB * Mega;
  int min_pages = divide_and_round_up(min_heap_bytes, page_size);
  int pages = round_up_to_power_of_two(min_pages); // necessary so heap bytes is power of two
  // lprintf("page_size %d, Mega %d, min_heap_bytes_for_all_cores %d, Logical_Core::group_size %d,  min_pages_per_core %d,  pages_per_core %d, pages_per_core * Logical_Core::group_size %d\n",
          // page_size, Mega, min_heap_bytes_for_all_cores, Logical_Core::group_size,  min_pages_per_core,  pages_per_core, pages_per_core * Logical_Core::group_size);
  return pages;
}

void Memory_System::initialize_from_snapshot(int32 snapshot_bytes, int32 sws, int32 fsf, int32 lastHash) {
  set_page_size_used_in_heap();
  
  int                  pages = calculate_total_pages(page_size);
  u_int32    total_heap_size = pages *  page_size;    
  
  snapshot_window_size.initialize(sws, fsf);

  heap_base = NULL;
  map_memory(getpid(), total_heap_size);

  initialize_memory();
  
  object_table = new Object_Table();

  init_buf ib = {
    snapshot_bytes, sws, fsf, lastHash,
    heap_base,
    unprotected_heap_base,
    page_size_used_in_heap, getpid(),
    object_table,
    global_GC_values
  };

  initialize_main(&ib);
}

void Memory_System::set_page_size_used_in_heap() {
  if (use_huge_pages) {
    if (!ask_Linux_for_huge_pages(calculate_total_pages(huge_page_size)))
      use_huge_pages = false;
  }
  lprintf("Using %s pages.\n", use_huge_pages ? "huge" : "normal");
  int hps = huge_page_size,  nps = normal_page_size; // compiler bug, need to alias these
  page_size_used_in_heap = use_huge_pages ? hps : nps;
  
  assert_always(page_size % page_size_used_in_heap == 0); // Page-aligned memory required! (see Memory_System.h)
}



void Memory_System::map_memory(int pid, size_t total_heap_size) {
  heap_base     = map_heap_memory(total_heap_size, total_heap_size, heap_base, 0, pid,  MAP_SHARED);
  heap_past_end = heap_base + total_heap_size;
  
  unprotected_heap_base     = map_heap_memory(total_heap_size, total_heap_size, unprotected_heap_base, 0, pid,  MAP_SHARED);
  unprotected_heap_offset   = unprotected_heap_base - heap_base;
  
  assert(heap_base < heap_past_end);
}

bool Memory_System::ask_Linux_for_huge_pages(int desired_huge_pages) {
  if (On_Apple || On_Intel_Linux || desired_huge_pages == 0)
    return true;

  int initially_available_huge_pages = how_many_huge_pages();
  if (initially_available_huge_pages >= desired_huge_pages) {
    lprintf("Linux has enough huges pages: %d >= %d\n", initially_available_huge_pages, desired_huge_pages);
    return true;
  }
  request_huge_pages(desired_huge_pages);
  int available_huge_pages = how_many_huge_pages();
  if ( available_huge_pages >= desired_huge_pages ) {
    lprintf("Started with %d huge pages, needed %d, acquired %d. Will use huge pages.\n",
            initially_available_huge_pages, desired_huge_pages, available_huge_pages);
    return true;
  }
  lprintf("Unable to procure huge_pages, started with %d, wanted %d, got %d; consider --huge_pages %d when starting tile-monitor. Reverting to normal pages. This will slow things down.\n",
          initially_available_huge_pages, desired_huge_pages, available_huge_pages, desired_huge_pages);
  return false;
}


static const char* hugepages_control_file = "/proc/sys/vm/nr_hugepages";

int Memory_System::how_many_huge_pages() {
  FILE* hpf = fopen(hugepages_control_file, "r");
  if (hpf == NULL) { perror("could not open nr_hugepages"); OS_Interface::die("nr_hugepages"); }
  int available_huge_pages = -1;
  fscanf(hpf, "%d%%", &available_huge_pages);
  fclose(hpf);
  return available_huge_pages;
}


void Memory_System::request_huge_pages(int desired_huge_pages) {
  FILE* hpf = fopen(hugepages_control_file, "w");
  if (hpf == NULL) { perror("could not open nr_hugepages"); OS_Interface::die("nr_hugepages"); }
  fprintf(hpf, "%d\n", desired_huge_pages);
  fclose(hpf);
}


void Memory_System::initialize_main(init_buf* ib) {
  // Each core homes its own shared Multicore_Object_Heap object
  // Each core has its own private Memory_System object
  // The actual memory for the heap is one contiguous address space, but each core uses a (page-sized) piece of it,
  // managed by each Multicore_Object_Heap object.


  // Create the Multicore_Object_Heap object on each core for homing.
  FOR_ALL_RANKS(i)
    if (i == Logical_Core::my_rank())
      create_my_heap(ib);
    else {
      logical_cores[i].message_queue.buffered_send_buffer(ib, sizeof(*ib));  // ensure that helper is delayed till now, even if Force_Direct_Memory_Access
      if (check_many_assertions)
        lprintf("finished sending init buffer\n");
      
      Logical_Core* sender;
      Multicore_Object_Heap** heaps_buf = (Multicore_Object_Heap**)Message_Queue::buffered_receive_from_anywhere(true, &sender, Logical_Core::my_core());
      heaps[i] = *heaps_buf;
      sender->message_queue.release_oldest_buffer(heaps_buf);
    }
  // don't need to ilib_mem_invalidate(p, nbytes) other read_mostly heaps because we haven't written anything to them yet

  if (check_many_assertions)
    lprintf("finished creating all heaps\n");

  if (Replicate_PThread_Memory_System || On_Tilera) {
    // Now, send the addresses of these.
    FOR_ALL_OTHER_RANKS(i)
      logical_cores[i].message_queue.buffered_send_buffer(&heaps[0],  sizeof(heaps));
  }

  if (check_many_assertions)
    lprintf("finished sending heap\n");

  object_table->pre_store_whole_enchillada();
}


// TODO: the implementation of this function breaks abstraction. It should use messages instead using directly the low-level functions
void Memory_System::initialize_helper() {
  Logical_Core* sender;
  init_buf* ib = (init_buf*)Message_Queue::buffered_receive_from_anywhere(true, &sender, Logical_Core::my_core());
  
  if (Replicate_PThread_Memory_System  ||  On_Tilera)
    init_values_from_buffer(ib); // not needed with common structure
  
  
  if (On_Tilera) {
    int                  pages = calculate_total_pages(page_size);
    u_int32    total_heap_size = pages *  page_size;     
    map_memory(ib->main_pid, total_heap_size);
  }
  
  create_my_heap(ib);
  
  sender->message_queue.release_oldest_buffer(ib);
  
  Logical_Core::main_core()->message_queue.buffered_send_buffer(&heaps[Logical_Core::my_rank()], sizeof(Multicore_Object_Heap*));
  if (check_many_assertions) lprintf("finished sending my heaps\n");

  if (!Replicate_PThread_Memory_System && !On_Tilera)
    return;
  
  void* heaps_buf = Message_Queue::buffered_receive_from_anywhere(true, &sender, Logical_Core::my_core());
  
  memcpy(&heaps, heaps_buf, sizeof(heaps));  
  sender->message_queue.release_oldest_buffer(heaps_buf);

  // don't need to ilib_mem_invalidate(p, nbytes) other read_mostly heaps because we haven't written anything to them yet
}



void Memory_System::init_values_from_buffer(init_buf* ib) {
  heap_base = ib->heap_base;
  unprotected_heap_base = ib->unprotected_heap_base;
  unprotected_heap_offset = unprotected_heap_base - heap_base;
  page_size_used_in_heap = ib->page_size;
  object_table = ib->object_table;
  
  assert(   ( Use_Object_Table && object_table)
         || (!Use_Object_Table && object_table == NULL));

  snapshot_window_size.initialize(ib->sws, ib->fsf);

  global_GC_values = ib->global_GC_values;
}


// memory system is private; but heaps is shared

void Memory_System::create_my_heap(init_buf* ib) {
  const int my_rank = Logical_Core::my_rank();

  Multicore_Object_Heap* h = new Multicore_Object_Heap();
  h->initialize_multicore(ib->lastHash + my_rank,
                          page_size_used_in_heap, On_Tilera );
  heaps[my_rank] = h;
}


// three phases (for read-mostly heaps); all machines pre-cohere all heaps, then scan, the all post-cohere

// We used to do each core's heap in parallel, but when we introduced the read-mostly heap
// went back to serial, because of intercore cache-line invalidation message deadlock worries.
// xxxxxx I bet we could go back to parallel. -- dmu 4/09

void Memory_System::scan_compact_or_make_free_objects_everywhere(bool compacting, Abstract_Mark_Sweep_Collector* gc_or_null) {
  
  enforce_coherence_before_each_core_stores_into_its_own_heap();
  scanCompactOrMakeFreeObjectsMessage_class m(compacting, gc_or_null);
  m.send_to_all_cores();
  enforce_coherence_after_each_core_has_stored_into_its_own_heap();
}


void Memory_System::scan_compact_or_make_free_objects_here(bool compacting, Abstract_Mark_Sweep_Collector* gc_or_null) {
  heaps[Logical_Core::my_rank()]->scan_compact_or_make_free_objects(compacting, gc_or_null);
}


static const char check_mark[4] = "sqi";


void Memory_System::save_to_checkpoint(FILE* f) {
  write_mark(f, check_mark);

  int32 len = strlen(image_name);
  xfwrite(&len, sizeof(len), 1, f);
  xfwrite(image_name, 1, len,  f);

  xfwrite(&Logical_Core::group_size, sizeof(Logical_Core::group_size), 1, f);

  xfwrite(this, sizeof(*this), 1, f);

  fatal("*TODO*: (the next) probably requires code modification.");
  FOR_ALL_RANKS(rank)
    heaps[rank]->save_to_checkpoint(f);

  object_table->save_to_checkpoint(f);
}


void Memory_System::restore_from_checkpoint(FILE* /* f */, int /* dataSize */, int /* lastHash */, int /* savedWindowSize */, int /* fullScreenFlag */) {
# if true
  assert_always_msg(false, "deactivated checkpointing until threadsafe memory_system is ready for Tilera");
# else

  lprintf("restoring memory system...\n");
  read_mark(f, check_mark);

  int32 len;
  xfread(&len, sizeof(len), 1, f);
  char buf[BUFSIZ];
  if (len >= BUFSIZ-1) fatal("");
  xfread(buf, 1, len, f);
  // Squeak_Image_Reader::imageNamePut_on_all_cores(buf, len);

  int32 gs;
  xfread(&gs, sizeof(gs), 1, f);
  if (gs != Logical_Core::group_size) fatal("group_size mismatch");

  initialize_from_snapshot(dataSize, savedWindowSize, fullScreenFlag, lastHash);

  Memory_System local_ms;

  xfread(&local_ms, sizeof(local_ms), 1, f);

  tl->growHeadroom = local_ms.growHeadroom;
  tl->shrinkThreshold = local_ms.shrinkThreshold;

  FOR_ALL_HEAPS(rank,mutability)
    heaps[rank][mutability]->restore_from_checkpoint(f);


  if (tl->read_write_memory_base != local_ms.read_write_memory_base) fatal("read_write_memory_base mismatch");
  if (tl->read_write_memory_past_end != local_ms.read_write_memory_past_end) fatal("memory_past_end mismatch");
  if (page_size_used_in_heap != local_ms.page_size_used_in_heap) fatal("page_size_used_in_heap mismatch");

  bcopy(local_ms.second_chance_core_for_allocation,
        tl->second_chance_core_for_allocation,
        sizeof(second_chance_core_for_allocation));
  tl->gcCount = local_ms.gcCount;
  tl->gcMilliseconds = local_ms.gcMilliseconds;
  tl->gcCycles = local_ms.gcCycles;

  object_table->restore_from_checkpoint(f);

  finished_adding_objects_from_snapshot();
# endif
}


void Memory_System::invalidate_heaps_and_fence(bool mine_too) {
  fatal("*TODO* requires update w/ single heap?");
  FOR_ALL_RANKS(i)
    if (mine_too  ||  i != Logical_Core::my_rank())
      heaps[i]->invalidate_whole_heap();
  OS_Interface::mem_fence();
}

void Memory_System::enforce_coherence_before_store_into_object_by_interpreter(void* p, int /* nbytes */, Object_p dst_obj_to_be_evacuated) {
  // to avoid deadlock caused by asking other cores to invalidate lines in the middle of interpreter and not being able to gc when another core asks me,
  // just move this object to read-write heap afterwards. Don't do enforce_coherence_before_store stuff.
  assert(contains(p));
  /*if (is_address_read_mostly(p))
    The_Squeak_Interpreter()->remember_to_move_mutated_read_mostly_object(dst_obj_to_be_evacuated->as_oop());*/
}


void Memory_System::pre_cohere(void* start, int nbytes) {
  if (nbytes == 0)  return;
  if (The_Squeak_Interpreter()->am_receiving_objects_from_snapshot()) return; // will be done at higher level
  // lprintf("pre_cohere start 0x%x %d\n", start, nbytes);

  if (!contains(start) && object_table->probably_contains_not(start)) {
    lprintf("pid %d, about_to_write_read_mostly_memory to bad address 0x%x 0x%x\n", getpid(), start, nbytes);
    fatal();
  }
  aboutToWriteReadMostlyMemoryMessage_class(start, nbytes).send_to_other_cores();

  // lprintf("pre_cohere end\n");
}


void Memory_System::post_cohere(void* start, int nbytes) {
  if (nbytes == 0)  return;
  if (The_Squeak_Interpreter()->am_receiving_objects_from_snapshot()) return; // will be done at higher level
  // lprintf(post_cohere start 0x%x %d\n", start, nbytes);
  OS_Interface::mem_flush(start, nbytes);
  OS_Interface::mem_fence();
  // lprintf("post_cohere end\n");
}


void Memory_System::enforce_coherence_after_this_core_has_stored_into_all_heaps() {
  FOR_ALL_RANKS(i)
    heaps[i]->enforce_coherence_in_whole_heap_after_store();
}


void Memory_System::do_all_oops_including_roots_here(Oop_Closure* oc, bool sync_with_roots)  {
  The_Interactions.do_all_roots_here(oc);
  
  FOR_EACH_OBJECT(obj)
    obj->do_all_oops_of_object(oc);
    
  if (sync_with_roots)
    The_Squeak_Interpreter()->sync_with_roots();
}



void Memory_System::print() {
  lprintf("Memory_System:n");
  lprintf("use_huge_pages: %d, min_heap_MB %d, replicate_methods %d, replicate_all %d\n"
                  "heap_base 0x%x, heap_past_end 0x%x, "
                  "page_size_used_in_heap %d, round_robin_period %d, \n"
                  "gcCount %d, gcMilliseconds %d, gcCycles %lld\n",
                  use_huge_pages, min_heap_MB, replicate_methods, replicate_all, 
                  heap_base, heap_past_end,
                  page_size_used_in_heap, round_robin_period,
                  global_GC_values->gcCount, global_GC_values->gcMilliseconds, global_GC_values->gcCycles);
  if ( object_table != NULL)
    object_table->print();

  print_heaps();
}


void Memory_System::print_heaps() {
  FOR_ALL_RANKS(rank) {
    lprintf("heap %d:\n", rank);
    heaps[rank]->print(stdout);
  }
}


# define DEF_SEC(T) \
void Memory_System::store_enforcing_coherence(T* p, T x, Object_p dst_obj_to_be_evacuated_or_null) { \
  if (sizeof(T) == bytes_per_oop) { DEBUG_STORE_CHECK((oop_int_t*)(p), (oop_int_t)(x)); } \
  assert(contains(p)); \
  *p = x; \
}


FOR_ALL_STORE_ENFORCING_COHERENCE_FUNCTIONS(DEF_SEC)


void Memory_System::store_bytes_enforcing_coherence(void* dst, const void* src, int nbytes,   Object_p dst_obj_to_be_evacuated_or_null) {
  assert(contains(dst));
  
  DEBUG_MULTIMOVE_CHECK(dst, src, nbytes / bytes_per_oop);
  
  memmove(dst, src, nbytes);
}


void Memory_System::store_2_enforcing_coherence(int32* p1, int32 i1, int32 i2,  Object_p dst_obj_to_be_evacuated_or_null) {
  assert(contains(p1));
  DEBUG_STORE_CHECK(p1, i1);
  DEBUG_STORE_CHECK(&p1[1], i2);
  *p1 = i1; 
  p1[1] = i2; 
}

int Memory_System::assign_rank_for_snapshot_object() {
  return round_robin_rank();
}


char  Memory_System::mmap_filename[BUFSIZ] = { 0 };


# if On_iOS

char* Memory_System::map_heap_memory(size_t total_size,
                                     size_t bytes_to_map,
                                     void*  where,
                                     off_t  offset,
                                     int    main_pid,
                                     int    flags) {
  return (char*)malloc(total_size);
}



# else

char* Memory_System::map_heap_memory(size_t total_size,
                                     size_t bytes_to_map,
                                     void*  where,
                                     off_t  offset,
                                     int    main_pid,
                                     int    flags) {
  assert_always(Max_Number_Of_Cores >= Logical_Core::group_size);
  
  assert( Memory_Semantics::cores_are_initialized());
  assert( On_Tilera || Logical_Core::running_on_main());
  
   
  const bool print = false;
  
  snprintf(mmap_filename, sizeof(mmap_filename), Memory_System::use_huge_pages ? "/dev/hugetlb/rvm-%d" : "/tmp/rvm-%d", main_pid);
  int open_flags = (where == NULL  ?  O_CREAT  :  0) | O_RDWR;
  
  int mmap_fd = open(mmap_filename, open_flags, 0600);
  if (mmap_fd == -1)  {
    char buf[BUFSIZ];
    snprintf(buf, sizeof(buf), "could not open mmap file, on %d, name %s, flags 0x%x",
            Logical_Core::my_rank(), mmap_filename, open_flags);
    perror(buf);
  }
  
  if (!Memory_System::use_huge_pages && ftruncate(mmap_fd, total_size)) {
    char buf[BUFSIZ];
    snprintf(buf, sizeof(buf), "The mmap-file could not be extended to the required heap-size. Requested size was %.2f MB. ftruncate", (float) total_size / 1024.0 / 1024.0);
    perror(buf);
    unlink(mmap_filename);
    fatal("ftruncate");
  }
  
  
  
  // Cannot use MAP_ANONYMOUS below because all cores need to map the same file
  void* mmap_result = mmap(where, bytes_to_map, PROT_READ | PROT_WRITE,  flags, mmap_fd, offset);
  if (check_many_assertions)
    lprintf("mmapp: address requested 0x%x, result 0x%x, bytes 0x%x, flags 0x%x, offset in file 0x%x\n",
            where, mmap_result, bytes_to_map, flags, offset);
  if (print)
    lprintf("mmap(<requested address> 0x%x, <byte count to map> 0x%x, PROT_READ | PROT_WRITE, <flags> 0x%x, open(%s, 0x%x, 0600), <offset> 0x%x) returned 0x%x\n",
            where, bytes_to_map, flags, mmap_filename, open_flags, offset, mmap_result);
  if (mmap_result == MAP_FAILED) {
    printf("error at mmap() (errno: %d: %s).\n", errno, strerror(errno));  
    char buf[BUFSIZ];
    snprintf(buf, sizeof(buf),
             "mmap failed on core %d. Requested %.2f MB for %s. mmap", 
             Logical_Core::my_rank(),
             (float)bytes_to_map / 1024.0 / 1024.0, 
             (where == NULL) ? "1st heap part" : "2nd heap part");
    perror(buf);
    unlink(mmap_filename);
    fatal("mmap");
  }
  if (where != NULL  &&  where != (void*)mmap_result) {
    lprintf("mmap asked for memory at 0x%x, but got it at 0x%x\n",
            where, mmap_result);
    fatal("mmap was uncooperative");
  }
  char* mem = (char*)mmap_result;
  close(mmap_fd);
  
  assert_always( mem != NULL );
  return mem;
}
# endif // On_iOS

/******************************** Page Support ********************************/

# define FOR_EACH_FREE_PAGE(page) \
for ( Page * page  = (Page*)global_GC_values->free_page; \
             page != NULL; \
             page  = page->next_free_page )

# define FOR_EACH_FREE_PAGEPTR(page_ptr) \
for ( Page ** page_ptr  = (Page**)&(global_GC_values->free_page); \
             *page_ptr != NULL; \
              page_ptr  = &((*page_ptr)->next_free_page) )


void Memory_System::initialize_memory() {
  int                  pages = calculate_total_pages(page_size);
  u_int32    total_heap_size = pages *  page_size;  
  
  global_GC_values->liveness =     (LPage *)Memory_Semantics::shared_malloc(sizeof(LPage) * pages);
  global_GC_values->livenessCopy = (LPage *)Memory_Semantics::shared_malloc(sizeof(LPage) * pages);
  for (int i = 0; i<pages; i++) {
    global_GC_values->liveness[i].setAllocated(false);
    global_GC_values->livenessCopy[i].setAllocated(false);
  }
  global_GC_values->adjustLivenessCopyForCore = (bool*)Memory_Semantics::shared_malloc(sizeof(bool) * Logical_Core::group_size);
  for(int i=0;i<Logical_Core::group_size;i++) {
    global_GC_values->adjustLivenessCopyForCore[i] = false;
  }  
  
  global_GC_values->free_page = heap_base;
  FOR_EACH_PAGE(page) {
    page->init();
  }
  
  assert_always(sizeof(Page) == page_size);
  
  //if (check_many_assertions)
    verify_memory_initialization();
}


void Memory_System::verify_memory_initialization() {
  int desired_pages = calculate_total_pages(page_size);

  int nobjects = 0;
  for (Chunk* cptr = (Chunk*)heap_base; 
       cptr < (Chunk*)heap_past_end; 
       ) {
    Object* optr = cptr->object_from_chunk();
    
    if (!optr->isFreeObject())
      fatal("=> Error traversing object memory\n");
    else
      nobjects++;
      
    cptr = optr->nextChunk();
  }
  
  int n2objects = 0;
  for (Chunk* cptr = (Chunk*)(heap_base + unprotected_heap_offset); 
       cptr < (Chunk*)(heap_past_end + unprotected_heap_offset); 
       ) {
    Object* optr = cptr->object_from_chunk();
    
    if (!optr->isFreeObject())
      fatal("=> Error traversing object memory\n");
    else
      n2objects++;
      
    cptr = optr->nextChunk();
  }  
  
  
  // Test page freelist traversal
  int npages = 0;
  FOR_EACH_FREE_PAGE(pptr) {
    npages++;
  }
  
  assert_always(desired_pages == npages && npages == nobjects && nobjects == n2objects);
          
  /* Page Freelist Tests */
  Page* p1 = allocate(heap_past_end - heap_base);
  assert_always(p1 == (Page*)heap_base);
  Page* p2 = allocate(page_size);
  assert_always(p2 == NULL);
  for (Page* p = p1; (char*)p < heap_past_end; p++) {
    free(p);
  }
  
  assert_always(p1 == allocate(page_size));     // ->2->3->4->5->...
  p2 = allocate(page_size);                     // ->3->4->5->...
  Page* p3 = allocate(page_size);               // ->4->5->...
  free(p2);                                     // ->2->4->5->...
  assert_always(allocate(page_size) == p2);     // ->4->5->...
  free(p2);                                     // ->2->4->5->...
  free(p3);                                     // ->2->3->4->5->...
  assert_always(p2 == allocate(page_size));     // ->3->4->5->...
  assert_always(p3 == allocate(page_size));     // ->4->5->...
  free(p1);                                     // ->1->4->5->...
  Page* p4 = allocate(page_size*2);             // ->1->...
  assert_always(p1 == allocate(page_size));     // ->...
  free(p1);
  free(p2);
  free(p3);
  free(p4); 
  free(p4 + 1);
  
  p1 = allocate(page_size);  
  startGCpreparation();
  p2 = allocate(page_size * 2);
  adjustLivenessCopyForCore(Logical_Core::my_rank(), false);  // *pass checkpoint*
  p3 = allocate(page_size * 2);                               // discarded in liveness copy
  LPage* livenessArray = stopGCpreparation();
  
  for(int i=0; i<calculate_total_pages(page_size); i++) {
    if (livenessArray[i].liveBytes != global_GC_values->liveness[i].liveBytes) {
      assert_always(i == 3 || i == 4);
    }
  }
  
  free(p1);
  free(p2);
  free(p2 + 1);
  free(p3);
  free(p3 + 1);
  
  for(int i=0; i<calculate_total_pages(page_size); i++) {
    assert_always(!global_GC_values->liveness[i].isAllocated());
  }
  
  if(Verbose_Debug_Prints) {
    fprintf(stdout, "Memory succesfully verified: Expected %d pages; got %d objects & %d pages, %d contiguous bytes available.\n", 
            desired_pages, nobjects, npages, (int)maxContiguousBytesLeft());
  }    
}

Page * Memory_System::firstPage() {
  return (Page*)heap_base;
}

void Memory_System::free(Page * p) {
  p->initialize(page_size);
  pushPage(p);
  if(Verbose_Debug_Prints) fprintf(stdout, "Freed page %d.\n",
                                   (int)p);   
}

void Memory_System::pushPage(Page * p) {
  OS_Interface::mutex_lock(global_GC_values->mutex); // atomic
  
  Page* prev = NULL;
  
  FOR_EACH_FREE_PAGE(page) {
    if (p < page)
      break;
    
    prev = page;
  }
  
  if (prev == NULL) {
    p->next_free_page = (Page*)global_GC_values->free_page;
    global_GC_values->free_page = (char*)p;
    
  } else {
    p->next_free_page = prev->next_free_page;
    prev->next_free_page = p;
    
  }
  
  global_GC_values->liveness[p - (Page*)heap_base].setAllocated(false);
  
  OS_Interface::mutex_unlock(global_GC_values->mutex);
}

Page* Memory_System::allocate(size_t minSize_bytes) {
  int contiguousPagesRequired = divide_and_round_up(minSize_bytes,page_size);
  
  OS_Interface::mutex_lock(global_GC_values->mutex); // atomic  
        
  Page**           first = (Page**)&(global_GC_values->free_page);
  int    contiguousPages = 0;

  
  FOR_EACH_FREE_PAGEPTR(pageptr) {
    if (*first + contiguousPages == *pageptr) { 
      contiguousPages++;
    } else {
      first = pageptr;
      contiguousPages = 1;
    }
    
    if (contiguousPages == contiguousPagesRequired)
      break;
  }
  
  Page* p = NULL;
  
  if (contiguousPages == contiguousPagesRequired) {
    p = *first;
    *first = (*first + contiguousPages - 1)->next_free_page;
    
    p->initialize(page_size * contiguousPages);
    
    global_GC_values->liveness[p - (Page*)heap_base].setAllocated(true);
    
    if (Logical_Core::my_rank() < Logical_Core::group_size                          // exclude the GC core
        && global_GC_values->adjustLivenessCopyForCore[Logical_Core::my_rank()])
      global_GC_values->livenessCopy[p - (Page*)heap_base].setAllocated(true);
  } 
  
  OS_Interface::mutex_unlock(global_GC_values->mutex);
  
  if(Verbose_Debug_Prints) fprintf(stdout, "Allocated %d pages to Core %d's heap for the request of %d bytes.\n",
                                   contiguousPages, Logical_Core::my_rank(), (int)minSize_bytes); 
  
  return p;
}

int Memory_System::freePages() {
  // optimizable w/ counter variable
  int c = 0;
  FOR_EACH_FREE_PAGE(p)
    c++;
  return c;
}

u_int32 Memory_System::maxContiguousBytesLeft() {
  Page* startPage = (Page*)global_GC_values->free_page;
  int   contiguousPages = 0;
  int   maxContiguousPages = 0;
  
  FOR_EACH_FREE_PAGE(p) {
    if (startPage + contiguousPages == p) {
      contiguousPages++;
    } else {
      maxContiguousPages = max(contiguousPages,maxContiguousPages);
      startPage = p;
      contiguousPages = 1;
    }
  }
  maxContiguousPages = max(contiguousPages,maxContiguousPages);
  
  return maxContiguousPages * page_size;
}

char * Memory_System::heap_past_used_end() {
  int pages = calculate_total_pages(page_size);  
  int heap_used_end_page = -1;

  for(int i=pages - 1;i>=0;i--) {
    if(global_GC_values->liveness[i].isAllocated()) {
      heap_used_end_page = i + 1;
      break;
    }
  }
  
  assert(heap_used_end_page != -1); // at least one page *should* be allocated
  
  return (char*)(firstPage() + heap_used_end_page);
}

/**
 *  Support for the Parallel GC.
 *    
 *    When a new GC cycle starts, the GC requires a liveness array copy which reflects all pages'
 *    status *before* that GC cycle started. However, because not all interpreter Cores simultaneously
 *    pass the GC-start-checkpoint (i.e., not all Cores simultaneously start a new GC cycle), the 
 *    liveness array copy should be updated appropriately until all Cores passed this checkpoint. 
 *
 *    As soon as a Core passes the checkpoint, it has started the GC cycle and further page
 *    allocations to this Core's Object Heap should no longer be reflected in the liveness 
 *    array copy for that GC cycle.
 *
 *    To obtain a correct liveness array copy:
 *      - the GC will request the Memory System to start the preparation for a new cycle *before* 
 *        making the interpreter Cores pass the GC-start-checkpoint,
 *        (cfr. startGCpreparation())
 *      - an interpreter Core will notify the Memory System as soon as it passes the checkpoint,
 *        (cfr. adjustLivenessCopyForCore())
 *      - and the GC will request the Memory System to stop the cycle preparation after each
 *        interpreter Core passed the checkpoint. At that point, it will obtain a correct liveness 
 *        array copy :-). (cfr. stopGCpreparation())
 */
 

void Memory_System::startGCpreparation() {
  OS_Interface::mutex_lock(global_GC_values->mutex); // atomic  

  int pages = calculate_total_pages(page_size);
  for(int i=0;i<pages;i++) {
    global_GC_values->livenessCopy[i] = global_GC_values->liveness[i];
  }
  
  for(int i=0;i<Logical_Core::group_size;i++) {
    global_GC_values->adjustLivenessCopyForCore[i] = true;
  }
  
  OS_Interface::mutex_unlock(global_GC_values->mutex);  
}

void Memory_System::adjustLivenessCopyForCore(int r, bool b) {
  OS_Interface::mutex_lock(global_GC_values->mutex); // atomic  
  
  global_GC_values->adjustLivenessCopyForCore[r] = b;
  
  OS_Interface::mutex_unlock(global_GC_values->mutex);  
}

LPage* Memory_System::stopGCpreparation() {
  OS_Interface::mutex_lock(global_GC_values->mutex); // atomic  

  for(int i=0;i<Logical_Core::group_size;i++) {
    global_GC_values->adjustLivenessCopyForCore[i] = false;
  }
  
  OS_Interface::mutex_unlock(global_GC_values->mutex);  

  return global_GC_values->livenessCopy;
}

/******************************************************************************/