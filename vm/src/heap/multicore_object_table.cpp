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

bool Multicore_Object_Table::replicate = false  &&  !Omit_Duplicated_OT_Overhead;
 // cannot dup if overhead omitted

Multicore_Object_Table::Entry* Multicore_Object_Table::Segment::construct_free_list() {
  Entry* r = NULL;
  for (int i = n - 1;  i >= 0;  --i) {
    words[i].i = oop_int_t(r);
    r = Entry::from_word_addr(&words[i]);
  }
  return r;
}

Multicore_Object_Table::Multicore_Object_Table() : Abstract_Object_Table() {
  turn = 0;
  FOR_ALL_RANKS(i) {
    first_segment[i] = NULL;
    first_free_entry[i] = NULL;
    lowest_address[i] = (void*)~0;
    lowest_address_after_me[i] = NULL;
    allocatedEntryCount[i] = entryCount[i] = allocationsSinceLastQuery[i] = entriesFreedSinceLastQuery[i] = 0;
  }
  Entry::verify_from_oop_optimization();
  OS_Interface::abort_if_error("Segment heap creation", OS_Interface::mem_create_heap_if_on_Tilera(&heap, replicate));
}

void Multicore_Object_Table::update_bounds(Segment* s, int rank) {
  void* start = (void*)s;
  void* end = (void*)&s[1];
  if (lowest_address[rank] >= start)  lowest_address[rank] = start;
  if (lowest_address_after_me[rank] <  end)  lowest_address_after_me[rank] = end;
}

void Multicore_Object_Table::update_segment_list(Segment* s, int rank  COMMA_DCL_ESB) {
  s->set_next(first_segment[rank]  COMMA_USE_ESB);
  first_segment[rank] = s;
}


void Multicore_Object_Table::update_free_list(Segment* s, int rank) {
  first_free_entry[rank] = s->construct_free_list();
  entryCount[rank] += Segment::n;
}


Multicore_Object_Table::Segment::Segment(Multicore_Object_Table* mot, int rank  COMMA_DCL_ESB) {
  h._rank = rank;
  if (mot != NULL) {
    mot->update_bounds(this, rank);
    mot->update_segment_list(this, rank  COMMA_USE_ESB);
    mot->update_free_list(this, rank);
  }
}

void* Multicore_Object_Table::Segment::operator new(size_t s) {
  void* p = OS_Interface::rvm_memalign(The_Memory_System()->object_table->heap, alignment_and_size, sizeof(Segment));
  assert(sizeof(Segment) <= alignment_and_size);
  if (p == NULL) fatal("OT Segment allocation");
  // xxxxxx Should home segments appropriately someday.
  if (!The_Squeak_Interpreter()->use_checkpoint()) bzero(p, sizeof(Segment));
  return p;
}



bool Multicore_Object_Table::is_on_free_list(Entry* e, int rank) {
  for (Entry* ee = first_free_entry[rank];  ee;  ee = ee->word()->get_entry())
    if (ee == e)
      return true;
  return false;
}

bool Multicore_Object_Table::is_OTE_free(Oop x) { return !The_Memory_System()->contains(word_for(x)->obj()); }


bool Multicore_Object_Table::verify_entry_address(Entry* e) {
  FOR_ALL_RANKS(r)
    for (Segment* p = first_segment[r];  p != NULL;  p = p->next())
        if (p->contains_entry(e)) return true;
  fatal("bad entry");
  return false;
}

bool Multicore_Object_Table::verify() {
  return verify_all_free_lists()  &&  verify_all_segments(false);
}

bool Multicore_Object_Table::verify_after_mark() {
  return verify_all_free_lists()  &&  verify_all_segments(true);
}

bool Multicore_Object_Table::verify_all_free_lists() {
  FOR_ALL_RANKS(r) verify_free_list(r);
  return true;
}

bool Multicore_Object_Table::verify_free_list(int rank) {
  bool ok = true;
  for (Entry* e = first_free_entry[rank];  e != NULL;  e = e->word()->get_entry())
    ok = e->verify_free_entry(this) && ok;
  return ok;
}

bool Multicore_Object_Table::verify_all_segments(bool live_ones_are_marked) {
  bool ok = true;
  FOR_ALL_RANKS(r)
    for (Segment* p = first_segment[r];  p != NULL;  p = p->next())
      ok = p->verify(this, live_ones_are_marked) && ok;
  return ok;
}

bool Multicore_Object_Table::Segment::verify(Multicore_Object_Table* ot, bool live_ones_are_marked) {
  bool ok = true;
  for ( Multicore_Object_Table::Entry* e = first_entry();  e < end_entry();  e = e->next() )
    ok = e->verify(ot, live_ones_are_marked) && ok;
  return ok;
}

bool Multicore_Object_Table::Entry::verify(Multicore_Object_Table* ot, bool live_ones_are_marked) {
  return is_free(ot) || verify_used_entry(live_ones_are_marked);
}


// Slow; only for verification
bool Multicore_Object_Table::Entry::is_free(Multicore_Object_Table* ot) {
  const bool go_faster = true; // less precise
  Entry* f = word()->get_entry();
  if (f == NULL)  return true; // last free entry
  if (go_faster) return ot->probably_contains(f);
  FOR_ALL_RANKS(r)
    for (Segment* p = ot->first_segment[r];  p != NULL;  p = p->next())
      if ( p->contains_entry(f) )
        return true;
  return false;
}

bool Multicore_Object_Table::Entry::verify_free_entry(Multicore_Object_Table* ot) {
  assert_always(is_free(ot));
  return true;
}

bool Multicore_Object_Table::Entry::verify_used_entry(bool live_ones_are_marked) {
  Object* obj = word()->obj();
  if (obj != NULL) {
    assert_always(The_Memory_System()->contains(obj));
    assert_always(!obj->is_marked() || live_ones_are_marked);
  }
  else
    fatal("no addr");
  return true;
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


static const char check_mark[4] = "mot", seg_cm[4] = "seg";

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


void Multicore_Object_Table::Segment::save_to_checkpoint(FILE* f, int rank) {
  Segment* me = this;
  write_mark(f, seg_cm);
  xfwrite(&rank, sizeof(rank), 1, f);
  xfwrite(&me, sizeof(me), 1, f);
  xfwrite(this, sizeof(*this), 1, f);
  // fprintf(stderr, "wrote a segment for %d at 0x%x, file is at 0x%x\n", rank, this, ftell(f));
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

void Multicore_Object_Table::Segment::restore_from_checkpoint(FILE* f, Segment* segs[], int n_segs) {
  read_mark(f, seg_cm);
  int rank;
  xfread(&rank, sizeof(rank), 1, f);

  Segment* s;
  xfread(&s, sizeof(s), 1, f);
  lprintf("restoring a segment\n");

  for (int i = 0;  i < n_segs;  ++i)
    if (segs[i] == s) {
      segs[i] = NULL;
      xfread(s, sizeof(*s), 1, f);
      // fprintf(stderr, "read a segment for %d at 0x%x, file is at 0x%x\n", rank, s, ftell(f));
      return;
    }
  fprintf(stderr, "segment mismatch for %d got %p, but wanted: ", rank, s);
  for (int i = 0;  i < n_segs;  ++i)
    if (segs[i] != NULL) fprintf(stderr, "%p%s", segs[i], i  <  n_segs - 1   ?  ", "  :  "\n");
  fatal("Segment mismatch");
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


void Multicore_Object_Table::Segment::set_next(Segment* s  COMMA_DCL_ESB) {
  if (!ESB_OR_FALSE || !Multicore_Object_Table::replicate) h._next = s;
  else {
    The_Memory_System()->pre_cohere_object_table(&h, sizeof(h));
    h._next = s;
    The_Memory_System()->post_cohere_object_table(&h, sizeof(h));
  }
}

void Multicore_Object_Table::Segment::print() {
  lprintf("\tSegment: first_entry 0x%x, end_entry 0x%x\n", first_entry(), end_entry());
}


void Multicore_Object_Table::check_for_debugging(Oop x) {
  if (!probably_contains((void*)x.bits())) {
    lprintf("object_for caught one\n");
    fatal("caught it");
  }
}

