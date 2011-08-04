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


class Abstract_Object_Table {
 public:
  Abstract_Object_Table() {}

  Oop allocate_OTE_for_object_in_snapshot(Object*)  { fatal(); return Oop::from_bits(0); }
  int rank_for_adding_object_from_snapshot(Oop)     { fatal(); return -1; }

  Oop allocate_oop_and_set_backpointer(Object*, int           COMMA_DCL_ESB) { fatal(); return Oop::from_bits(0); }
  Oop allocate_oop_and_set_preheader(Object* /* obj */, int   COMMA_DCL_ESB) { fatal(); return Oop::from_bits(0); }

  void prepare_for_objects(int /* about_how_many */) { }

  void save_baseHeader(Oop, Object*)    { fatal(); }
  void restore_baseHeader(Oop, Object*) { fatal(); }

  Object* local_object_for(Oop) { fatal(); return NULL; }
  void set_local_object_for(Oop, Object*) { fatal(); }

  bool is_local_copy_in_use(Oop) { fatal(); return false; }

  Object* global_object_for(Oop) { fatal(); return NULL; }
  Object* global_object_for_unchecked(Oop) { fatal(); return NULL; }
  void set_global_object_for(Oop, Object*, bool /* do_check = true */ ) { fatal(); }

  Object* object_for(Oop) { fatal(); return NULL; }
  Object* object_for_unchecked(Oop) { fatal(); return NULL; }
  void set_object_for(Oop, Object_p, bool /* do_check = true */ ) { fatal(); }

  bool spare_bit_for(Oop)  { fatal(); return false; }
  void set_spare_bit_for(Oop, bool /* dont */ )  { fatal(); }

  void save_to_checkpoint(FILE*) { fatal(); }
  static bool restore_from_checkpoint(FILE*) { fatal(); return false; }



  void do_in_reverse_allocation_order(Oop_Closure*) { fatal(); }

  void clear_words() { fatal(); }

  void free_oop(Oop) { fatal(); }

  bool verify() { fatal(); return false; }
  bool verify_no_local_objects() { fatal(); return false; }
  bool verify_after_mark() { fatal(); return false; }

  bool probably_contains(void*)     const { fatal(); return false; }
  bool probably_contains_not(void*) const { fatal(); return false; }

  Oop get_stats(int) { fatal(); return Oop::from_bits(0); }

  void print() { fatal(); }

 protected:
};

