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

# if Use_Object_Table

bool Multicore_Object_Table::verify_after_mark() {
  return verify_all_free_lists()  &&  verify_all_segments(true);
}


Oop Multicore_Object_Table::get_stats(int rank) {
  int s = The_Squeak_Interpreter()->makeArrayStart();
  PUSH_POSITIVE_32_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(allocatedEntryCount[rank]);
  PUSH_POSITIVE_32_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(entryCount[rank]);
  PUSH_POSITIVE_32_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(allocationsSinceLastQuery[rank]);
  PUSH_POSITIVE_32_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(entriesFreedSinceLastQuery[rank]);
  allocationsSinceLastQuery[rank] = entriesFreedSinceLastQuery[rank] = 0;
  return The_Squeak_Interpreter()->makeArray(s);
}

Multicore_Object_Table::Multicore_Object_Table() : Segmented_Object_Table() {}


static const char check_mark[4] = "mot";

# define FOR_EACH_SEGMENT(s) \
  FOR_ALL_RANKS(r) \
    for (Segment* s = first_segment[r];  s != NULL;  s = s->next()) \

void Multicore_Object_Table::save_to_checkpoint(FILE* f) {
  write_mark(f, check_mark);
  xfwrite(this, sizeof(*this), 1, f);
  int n_segs = 0;
  FOR_EACH_SEGMENT(s) ++n_segs;
  xfwrite(&n_segs, sizeof(n_segs), 1, f);

  FOR_EACH_SEGMENT(s) s->save_to_checkpoint(f, r);

}


bool Multicore_Object_Table::is_OTE_free(Oop x) {
  return !The_Memory_System()->contains(word_for(x)->obj());
}




void Multicore_Object_Table::restore_from_checkpoint(FILE* f) {
  lprintf("restoring object table...\n");
  read_mark(f, check_mark);
  xfread(this, sizeof(*this), 1, f);
  int n_segs = -1;
  xfread(&n_segs, sizeof(n_segs), 1, f);
  Segment** segs = (Segment**)alloca(n_segs * sizeof(Segment*));
  for (int i = 0;  i < n_segs;  ++i)
    segs[i] = new Segment(NULL, -1  COMMA_FALSE_OR_NOTHING);

  for (int i = 0;  i < n_segs;  ++i)
    Segment::restore_from_checkpoint(f, segs, n_segs);
}


void Multicore_Object_Table::pre_store_whole_enchillada() {
  if (!replicate) return;
  FOR_ALL_RANKS(rank)
    for (Segment* p = first_segment[rank];  p != NULL;  p = p->next())
      The_Memory_System()->pre_cohere_object_table(p, sizeof(*p));
}

void Multicore_Object_Table::post_store_whole_enchillada() {
  if (!replicate) return;
  FOR_ALL_RANKS(rank)
    for (Segment* p = first_segment[rank];  p != NULL;  p = p->next())
      The_Memory_System()->post_cohere(p, sizeof(*p));
}

void Multicore_Object_Table::word_union:: pre_cohere_OTE() {
  The_Memory_System()-> pre_cohere_object_table(this, sizeof(*this));
}
void Multicore_Object_Table::word_union::post_cohere_OTE() {
  The_Memory_System()->post_cohere_object_table(this, sizeof(*this));
}



void Multicore_Object_Table::print() {
  FOR_ALL_RANKS(r) {
    lprintf("Multicore_Object_Table: rank %d,  ", r);
    lprintf("first_segment 0x%x, first_free_entry 0x%x, allocatedEntryCount %d, entryCount %d, allocationsSinceLastQuery %d, entriesFreedSinceLastQuery %d, lowest_address 0x%x, lowest_address_after_me 0x%x\n",
     first_segment[r], first_free_entry[r], allocatedEntryCount[r], entryCount[r], allocationsSinceLastQuery[r], entriesFreedSinceLastQuery[r], lowest_address[r], lowest_address_after_me[r]);
     for (Segment* s = first_segment[r];  s != NULL;  s = s->next())
       s->print();
  }
}

# endif
