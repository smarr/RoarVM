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


class Segmented_Object_Table : public Abstract_Object_Table {
public:
  static bool replicate;

protected:
  class Entry;
  
  void* lowest_address[Max_Number_Of_Cores];
  void* lowest_address_after_me[Max_Number_Of_Cores];
  
  static const int bit_mask = 3;
  static const int obj_mask = ~bit_mask;
  static const int spare_bit = 1;
  
  union word_union {
# if Extra_OTE_Words_for_Debugging_Block_Context_Method_Change_Bug
    struct { oop_int_t x, y, z, t; };
# endif
    oop_int_t i;
    Entry* _e;
    Entry* get_entry() { return _e; }
    void set_entry(Entry* e COMMA_DCL_ESB) { set(oop_int_t(e)  COMMA_USE_ESB); }// async
    
    Object* obj() { return (Object*)(i & obj_mask); }
    
    void set_obj(Object* x  COMMA_DCL_ESB)  {
      set(((oop_int_t)x & obj_mask)  |  (i & bit_mask)  COMMA_USE_ESB);
    }
    int bits() {  return i & bit_mask; }
    void set_bits(int x  COMMA_DCL_ESB) { set((i & obj_mask)  |  (x & bit_mask)  COMMA_USE_ESB); }
    bool get_spare_bit() {
      return i & spare_bit;
    }
    void set_spare_bit(bool x  COMMA_DCL_ESB) {
      if (Omit_Spare_Bit) return;
      if (x)  set(i |  spare_bit  COMMA_USE_ESB);
      else    set(i & ~spare_bit  COMMA_USE_ESB);
    }
    void set_obj_and_spare_bit(Object* obj,  bool spare  COMMA_DCL_ESB) {
      set((oop_int_t(obj) & ~bit_mask)  |  (spare ? spare_bit : 0)  COMMA_USE_ESB);
    }
    void set(oop_int_t x  COMMA_DCL_ESB) {
      if (!ESB_OR_FALSE  ||  !replicate) i = x;
      else { pre_cohere_OTE();  i = x;  post_cohere_OTE(); }
    }
    void  pre_cohere_OTE();
    void post_cohere_OTE();
    void clear_debugging_words() {
#     if Extra_OTE_Words_for_Debugging_Block_Context_Method_Change_Bug
      y = z = t = 0;
#     endif
    }
  };
  
  class Segment {
    struct header {
      Segment* _next;
      int _rank;
    } h;
    static const uint32_t alignment_and_size = PAGE_SIZE; // needed to find rank and later, for homing
  public:
    Segment* next() { return h._next; }
    int rank() { return h._rank; }
    static Segment* enclosing(void* p) { return (Segment*) ( int(p) & ~(alignment_and_size - 1)); }
    void set_next(Segment* s  COMMA_DCL_ESB);
    static const int n = (alignment_and_size - sizeof(header)) / sizeof(word_union);
    // parallel arrays to optimize caching, system uses word shift, see bits_for_hash in oop.h
    union word_union words[n];
    
    void* operator new(size_t);
    void operator delete(void *);
    Segment(Object_Table*,int  COMMA_DCL_ESB);
    Entry* construct_free_list();
    Entry*   end_entry() { return Entry::from_word_addr(&words[n]); }
    Entry*  last_entry() { return Entry::from_word_addr(&words[n-1]); }
    Entry* first_entry() { return Entry::from_word_addr(&words[0]); }
    bool contains_entry(Entry* e) { return first_entry() <= e  &&  e < end_entry(); }
    
    bool verify(Object_Table*, bool);
    
    void save_to_checkpoint(FILE*, int);
    static void restore_from_checkpoint(FILE*, Segment* segs[], int n);
    void print();
  };
  
  class Entry {
  public:
    static Entry* from_word_addr(word_union* x) { return (Entry*)x; }
    word_union* word() { return (word_union*)this; }
    oop_int_t mem_bits() { return (oop_int_t)this / sizeof(Object*); }
    static Entry* from_mem_bits(oop_int_t x) { return (Entry*)(x * sizeof(Object*)); }
    static Entry* from_oop(Oop x) {
      return /*from_mem_bits(x.mem_bits())*/ (Entry*)x.bits();
    }
    static bool verify_from_oop_optimization() {
      assert_always((int)from_mem_bits(Oop::from_bits(Oop::Illegals::magic).mem_bits()) == Oop::Illegals::magic);
      return true;
    }
    Entry* prev() { return from_word_addr(word() - 1); }
    Entry* next() { return from_word_addr(word() + 1); }
    Oop oop() { return Oop::from_mem_bits(mem_bits()); }
    int rank() { return enclosing_segment()->rank(); }
    Segment* enclosing_segment() { return Segment::enclosing(this); }
    
    bool is_free(Object_Table*);
    
    bool is_used();
    bool verify_free_entry(Object_Table*);
    bool verify_used_entry(bool);
    bool verify(Object_Table*, bool);
  };


protected:
  // need to be able to save and return a word  
  word_union* word_for(Oop x) {
    return entry_from_oop(x)->word();
  }
  

  OS_Interface::OS_Heap heap;
  
  Segment* first_segment[Max_Number_Of_Cores];
  Entry*   first_free_entry[Max_Number_Of_Cores];
  
  u_int32 allocatedEntryCount[Max_Number_Of_Cores];
  u_int32 entryCount[Max_Number_Of_Cores];
  u_int32 allocationsSinceLastQuery[Max_Number_Of_Cores];
  u_int32 entriesFreedSinceLastQuery[Max_Number_Of_Cores]; // how many frees have happened
  
  void add_entry_to_free_list(Entry* e, int rank  COMMA_DCL_ESB) {
    assert( e->word()->obj() == NULL);
    Entry*& first_free = first_free_entry[rank];
    e->word()->set_entry(first_free  COMMA_USE_ESB);
    first_free = e;
    --allocatedEntryCount[rank];
  }
  
  Entry* entry_from_oop(Oop x) {
    Entry* e = Entry::from_oop(x);
    if (check_many_assertions)
      verify_entry_address(e);
    return e;
  }

  void check_for_debugging(Oop x);
  
public:
  Segmented_Object_Table();
  
  Object* object_for(Oop x) {
    if (check_many_assertions) check_for_debugging(x);
    return word_for(x)->obj();
  }
  
  void set_object_for(Oop x, Object_p obj  COMMA_DCL_ESB) {
    word_for(x)->set_obj(obj  COMMA_USE_ESB);
  }
  
  
  inline Oop allocate_OTE_for_object_in_snapshot(Object*);
  inline int rank_for_adding_object_from_snapshot(Oop);

  
  void cleanup();
  
  bool verify();
  
protected:
  bool verify_free_list(int rank);
  bool verify_all_segments(bool);
  
  bool verify_all_free_lists();

  Oop allocate_oop(int rank COMMA_DCL_ESB);
  
private:
  
  bool verify_entry_address(Entry*);
  
  void update_bounds(Segment*, int);
  void update_segment_list(Segment*, int  COMMA_DCL_ESB);
  void update_free_list(Segment*, int);
  
  bool is_on_free_list(Entry*, int);

};
