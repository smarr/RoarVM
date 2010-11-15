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


class Multicore_Object_Heap: public Abstract_Object_Heap {
  int lastHash;

  public:
  void* operator new(size_t size);

  void initialize_multicore(int hash, char* mem, int size, int page_size, bool do_homing);
  private:
  void home_to_this_tile(int);
  bool verify_homing(int);
  public:

  int heap_byte_size() { return fatal("innapropriate"); }
  void* allocate_my_space(int) { return (void*)fatal("innapropriate"); }

  inline int32 newObjectHash();
  void set_lastHash(int x) { lastHash = x; }
  int  get_lastHash() { return lastHash; }
  inline Object_p allocate(oop_int_t byteSize, oop_int_t hdrSize,
                          oop_int_t baseHeader, Oop classOop, oop_int_t extendedSize, bool doFill = false,
                          bool fillWithNill = false);
  inline Chunk* allocateChunk_for_a_new_object(oop_int_t total_bytes);
  void add_object_from_snapshot(Oop, Object*, Object*);
  void flushExternalPrimitives();
  void handle_low_space_signal();



  Oop next_instance_of_after(Oop, Oop);

  void snapshotCleanUp();
  void write_image_file(FILE*, u_int32*, bool&);

  Object_p object_address_unchecked(Oop);

  void set_lowSpaceThreshold(int x) { lowSpaceThreshold = x; }
  int32 get_lowSpaceThreshold()  { return lowSpaceThreshold; }

  Oop get_stats();

  void save_to_checkpoint(FILE* f);
  void restore_from_checkpoint(FILE* f);

  // Unlike check_store, etc., this is used for the inchorent heap, so even integer oops matter
  // See memory_system declarations of store_enforcing_coherence
  inline void      store_enforcing_coherence(Oop* p, Oop x, Object_p dst_obj_to_be_evacuated_or_null);
  inline void      store_enforcing_coherence(oop_int_t*, oop_int_t, Object_p dst_obj_to_be_evacuated_or_null);
  inline void      store_bytes_enforcing_coherence(void*, const void*, int, Object_p dst_obj_to_be_evacuated_or_null);

  void print(FILE*);


};

