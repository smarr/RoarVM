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


inline Oop Multicore_Object_Table::allocate_oop(int rank COMMA_DCL_ESB)  {
  if (check_many_assertions  &&  The_Squeak_Interpreter()->is_initialized())
    verify_free_list(rank);
  
  Entry*& first_free = first_free_entry[rank];
  Entry* e = first_free;
  if (e == NULL) {
    new Segment(this, rank  COMMA_USE_ESB);
    e = first_free;
   }
  __attribute__((unused)) Entry* last_first_free = e; // debugging
  first_free = (Entry*)e->word()->get_entry();

  e->word()->clear_debugging_words();

  return Oop::from_mem_bits(e->mem_bits());
}



inline Oop Multicore_Object_Table::allocate_OTE_for_object_in_snapshot(Object*)  {
  int rank = The_Memory_System()->assign_rank_for_snapshot_object();
  return  allocate_oop(rank COMMA_FALSE_OR_NOTHING);
}


inline int Multicore_Object_Table::rank_for_adding_object_from_snapshot(Oop x) { return entry_from_oop(x)->rank(); }



inline Oop Multicore_Object_Table::allocate_oop_and_set_backpointer(Object_p obj, int rank  COMMA_DCL_ESB) {
  Oop r = allocate_oop(rank COMMA_USE_ESB);
  obj->set_backpointer(r); // should never be a read-mostly obj anyway
  set_object_for(r, obj  COMMA_USE_ESB);
  ++allocatedEntryCount[rank];
  ++allocationsSinceLastQuery[rank];
  return r;
}

inline Oop Multicore_Object_Table::allocate_oop_and_set_preheader(Object_p obj, int r  COMMA_DCL_ESB) { 
  obj->init_extra_preheader_word();
  return allocate_oop_and_set_backpointer(obj, r  COMMA_USE_ESB); 
}




inline bool Multicore_Object_Table::Entry::is_used() {
  Object* ow = word()->obj();
  return The_Memory_System()->contains(ow);
}

inline bool Multicore_Object_Table::probably_contains(void* p) const {
  if (The_Memory_System()->contains(p))
    return false;
  FOR_ALL_RANKS(r)
  if (lowest_address[r] <= p  &&  p  < lowest_address_after_me[r])
    return true;
  return false;
}

