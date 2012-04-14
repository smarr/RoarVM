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


# if Use_Object_Table

class Multicore_Object_Table: public Segmented_Object_Table {
public:
  void* operator new(size_t size) {
    return Memory_Semantics::shared_malloc(size);
  }
  Multicore_Object_Table();


  inline Oop allocate_oop_and_set_backpointer(Object_p obj, int rank  COMMA_DCL_ESB);
  inline Oop allocate_oop_and_set_preheader(Object_p obj, int r   COMMA_DCL_ESB);





public:

  bool spare_bit_for(Oop x)  { return word_for(x)->get_spare_bit(); }
  void set_spare_bit_for(Oop x, bool spare  COMMA_DCL_ESB)  { word_for(x)->set_spare_bit(spare  COMMA_USE_ESB); }


  void free_oop(Oop x  COMMA_DCL_ESB) {
    Entry* e = entry_from_oop(x);
    int rank = e->rank();
    e->word()->set_obj_and_spare_bit(NULL, false  COMMA_USE_ESB);
    add_entry_to_free_list(e, rank  COMMA_USE_ESB);
    --allocatedEntryCount[rank];
    ++entriesFreedSinceLastQuery[rank];
  }

  bool is_OTE_free(Oop x);
  
# if Extra_OTE_Words_for_Debugging_Block_Context_Method_Change_Bug
  void set_dbg_y(Oop x, oop_int_t m) { word_for(x)->y = m; }
  void set_dbg_z(Oop x, oop_int_t m) { word_for(x)->z = m; }
  void set_dbg_t(Oop x, oop_int_t m) { word_for(x)->t = m; }

  oop_int_t get_dbg_y(Oop x) { return word_for(x)->y; }
  oop_int_t get_dbg_z(Oop x) { return word_for(x)->z; }
  oop_int_t get_dbg_t(Oop x) { return word_for(x)->t; }
# endif


public:
  bool verify_after_mark();

  inline bool probably_contains(void*) const;
  inline bool probably_contains_not(void* obj) const {
    return not probably_contains(obj);
  }

  Oop get_stats(int);

public:
  void save_to_checkpoint(FILE*);
  void restore_from_checkpoint(FILE*);

  void  pre_store_whole_enchillada();
  void post_store_whole_enchillada();


  void print();

};

# endif // if Use_Object_Table

