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

  Oop allocate_OTE_for_object_in_snapshot(Object*) { return Oop::from_bits(fatal()); }
  int rank_for_adding_object_from_snapshot(Oop) { fatal(); return -1; }

  Oop allocate_oop_and_set_backpointer(Object*, int COMMA_DCL_ESB) { return Oop::from_bits(fatal()); }
  Oop allocate_oop_and_set_preheader(Object* obj, int  COMMA_DCL_ESB) { return Oop::from_bits(fatal()); }

  void prepare_for_objects(int /* about_how_many */) { }

  void save_baseHeader(Oop, Object*) { fatal(); }
  void restore_baseHeader(Oop, Object*) { fatal(); }

  Object* local_object_for(Oop) { return (Object*)fatal(); }
  void set_local_object_for(Oop, Object*) { fatal(); }

  bool is_local_copy_in_use(Oop) { return fatal(); }

  Object* global_object_for(Oop) { return (Object*)fatal(); }
  Object* global_object_for_unchecked(Oop) { return (Object*)fatal(); }
  void set_global_object_for(Oop, Object*, bool /* do_check = true */ ) { fatal(); }

  Object* object_for(Oop) { return (Object*)fatal(); }
  Object* object_for_unchecked(Oop) { return (Object*)fatal(); }
  void set_object_for(Oop, Object_p, bool /* do_check = true */ ) { fatal(); }

  bool spare_bit_for(Oop)  { return fatal(); }
  void set_spare_bit_for(Oop, bool /* dont */ )  { fatal(); }

  void save_to_checkpoint(FILE*) { fatal(); }
  static bool restore_from_checkpoint(FILE*) { return fatal(); }



  void do_in_reverse_allocation_order(Oop_Closure*) { fatal(); }

  void clear_words() { fatal(); }

  void free_oop(Oop) { fatal(); }

  bool verify() { return fatal(); }
  bool verify_no_local_objects() { return fatal(); }
  bool verify_after_mark() { return fatal(); }

  bool probably_contains(void*) { return fatal(); }

  Oop get_stats(int) { return Oop::from_bits(fatal()); }

  void print() { fatal(); }

 protected:
};

