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


inline Oop Segmented_Object_Table::allocate_OTE_for_object_in_snapshot(Object*)  {
  int rank = The_Memory_System()->assign_rank_for_snapshot_object();
  return  allocate_oop(rank COMMA_FALSE_OR_NOTHING);
}


inline int Segmented_Object_Table::rank_for_adding_object_from_snapshot(Oop x) {
  return entry_from_oop(x)->rank();
}


inline Oop Segmented_Object_Table::allocate_oop(int rank COMMA_DCL_ESB)  {
  if (check_many_assertions  &&  The_Squeak_Interpreter()->is_initialized()) verify_free_list(rank);
  Entry*& first_free = first_free_entry[rank];
  Entry* e = first_free;
  if (e == NULL) {
    new Segment((Object_Table*)this, rank  COMMA_USE_ESB);
    e = first_free;
  }
  __attribute__((unused)) Entry* last_first_free = e; // debugging
  first_free = (Entry*)e->word()->get_entry();
  
  e->word()->clear_debugging_words();
  
  return Oop::from_mem_bits(e->mem_bits());
}

