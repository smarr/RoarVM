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


class Abstract_Object_Heap {
 protected:

  Oop* _start;
  Oop* _next;
  Oop* _end;

 public:
  int allocationsSinceLastQuery;
  int compactionsSinceLastQuery;

  int32 lowSpaceThreshold;

  Abstract_Object_Heap() {
    _start = _next = _end = NULL; lowSpaceThreshold = 0;
    allocationsSinceLastQuery = compactionsSinceLastQuery = 0;
  }
  bool is_initialized() { return _start != NULL; }


  virtual int heap_byte_size() = 0;
  virtual void* allocate_my_space(int) = 0;
  virtual void saveProcessSignallingLowSpace() {}

  // make these static as an optimization for how


  static void check_store( Oop* /* p */) {
    // gc
  }
  void check_multiple_stores_for_generations_only( Oop dsts[], oop_int_t n);
  void multistore( Oop* dst, Oop* src, oop_int_t n);
  void multistore( Oop* dst, Oop  src, oop_int_t n);
  void multistore( Oop* dst, Oop* end, Oop src);
  static void record_class_header(Object* /* obj */, Oop klass) { if (klass.is_new()) unimplemented();  }

  static void possibleRootStore(Oop /* holder */, Oop /* contents */) { /*unimplemented*/ }
  static void clearRootsTable() { lprintf("no clearRootsTable()\n"); }


 public:
  virtual void initialize();
  virtual void initialize(void* mem, int size);


  bool sufficientSpaceToAllocate(oop_int_t bytes);
  Chunk* allocateChunk(oop_int_t total_bytes);
  virtual Object_p object_address_unchecked(Oop) = 0;

  Object* accessibleObjectAfter(Object*);
  Object* firstAccessibleObject();
  Oop     initialInstanceOf(Oop);

  Object* first_object_or_null();
  inline Object*  next_object(Object*);
  Object*   end_objects() const { return (Object*)_next; } // addr past objects
  Oop*     end_of_space() { return (Oop*)_end; }

  Object* first_object_without_preheader();
  Object* next_object_without_preheader(Object*);
  Object*  end_objects_without_preheader() { return (Object*)_next; } // addr past objects

  u_int32 bytesLeft(bool includeSwapSpace = false) { return (char*)_end - (char*)_next; }
  int bytesUsed() { return (char*)_next - (char*)_start; }

  void     set_end_objects(Oop* x) {
    Oop* old_next = check_many_assertions ? _next : NULL;
    assert(x >= _start);
    _next = x;
    if (check_many_assertions &&  _next < old_next)
      for (Oop* p = x;  p < old_next;  ++p)
        *p = Oop::from_bits(Oop::Illegals::trimmed_end);
  }



  Oop* startOfMemory() const { return _start; }

  bool contains(void* p) { Oop* pp = (Oop*)p;  return _start <= pp  &&  pp < _next; }

  u_int32    approx_object_count() {
    return bytesUsed() / sizeof(Oop) / 10;
  }
  
  inline int rank();


  bool verify();
  bool verify_address_in_heap(void*);
  void ensure_all_unmarked();

  void zap_unused_portion();
  void scan_compact_or_make_free_objects(bool compacting, Abstract_Mark_Sweep_Collector* gc_or_null);


  virtual Oop get_stats() = 0;

  inline bool is_read_mostly();
  inline bool is_read_write();

  inline void enforce_coherence_before_store(void*, int nbytes);
  inline void enforce_coherence_after_store(void*, int nbytes);

  void enforce_coherence_before_store_into_object_by_interpreter(void*, int nbytes, Object_p dst);
  void enforce_coherence_after_store_into_object_by_interpreter(void*, int nbytes);

  void enforce_coherence_in_whole_heap_before_store() { enforce_coherence_before_store(startOfMemory(), bytesUsed()); }
  void enforce_coherence_in_whole_heap_after_store()  { enforce_coherence_after_store (startOfMemory(), bytesUsed()); }

  void invalidate_whole_heap() { OS_Interface::invalidate_mem(startOfMemory(), bytesUsed()); }

  void do_all_oops(Oop_Closure*);

  void print(FILE* f = stdout);

 private:
  Object* object_from_chunk(Chunk*);
  Object* object_from_chunk_without_preheader(Chunk*);
};

# define FOR_EACH_OBJECT_IN_HEAP(a_heap, object_ptr) \
for ( Object* object_ptr  =  (a_heap)->first_object_or_null(); \
              object_ptr !=  NULL; \
              object_ptr  =  (a_heap)->next_object(object_ptr) )

