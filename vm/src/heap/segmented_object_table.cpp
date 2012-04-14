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


# include "headers.h"


bool Segmented_Object_Table::replicate = false  &&  !Omit_Duplicated_OT_Overhead;
                                         // cannot dup if overhead omitted


Segmented_Object_Table::Entry* Segmented_Object_Table::Segment::construct_free_list() {
  Entry* r = NULL;
  for (int i = n - 1;  i >= 0;  --i) {
    words[i].i = oop_int_t(r);
    r = Entry::from_word_addr(&words[i]);
  }
  return r;
}


Segmented_Object_Table::Segmented_Object_Table() : Abstract_Object_Table() {
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


void Segmented_Object_Table::cleanup() {
  FOR_ALL_RANKS(r) {
    Segment* next;
    for (Segment* s = first_segment[r];  s != NULL;  s = next) {
      next = s->next();
      delete s;
    }
  }
}


void Segmented_Object_Table::update_bounds(Segment* s, int rank) {
  void* start = (void*)s;
  void* end = (void*)&s[1];
  if (lowest_address[rank] >= start)  lowest_address[rank] = start;
  if (lowest_address_after_me[rank] <  end)  lowest_address_after_me[rank] = end;
}


void Segmented_Object_Table::update_segment_list(Segment* s, int rank  COMMA_DCL_ESB) {
  s->set_next(first_segment[rank]  COMMA_USE_ESB);
  first_segment[rank] = s;
}


void Segmented_Object_Table::update_free_list(Segment* s, int rank) {
  first_free_entry[rank] = s->construct_free_list();
  entryCount[rank] += Segment::n;
}


Segmented_Object_Table::Segment::Segment(Object_Table* mot, int rank  COMMA_DCL_ESB) {
  h._rank = rank;
  if (mot != NULL) {
    mot->update_bounds(this, rank);
    mot->update_segment_list(this, rank  COMMA_USE_ESB);
    mot->update_free_list(this, rank);
  }
}


void* Segmented_Object_Table::Segment::operator new(size_t /* s */) {
  void* p = OS_Interface::rvm_memalign_shared(The_Memory_System()->object_table->heap, alignment_and_size, sizeof(Segment));
  assert(sizeof(Segment) <= alignment_and_size);
  if (p == NULL) fatal("OT Segment allocation");
  // xxxxxx Should home segments appropriately someday.
  if (!The_Squeak_Interpreter()->use_checkpoint()) bzero(p, sizeof(Segment));
  return p;
}


void Segmented_Object_Table::Segment::operator delete(void * mem) {
  OS_Interface::rvm_free_aligned_shared(mem);
}



bool Segmented_Object_Table::is_on_free_list(Entry* e, int rank) {
  for (Entry* ee = first_free_entry[rank];  ee;  ee = ee->word()->get_entry())
    if (ee == e)
      return true;
  return false;
}


bool Segmented_Object_Table::verify_entry_address(Entry* e) {
  FOR_ALL_RANKS(r)
  for (Segment* p = first_segment[r];  p != NULL;  p = p->next())
    if (p->contains_entry(e)) return true;
  fatal("bad entry");
  return false;
}


bool Segmented_Object_Table::verify() {
  if (this == NULL) {
    assert(!Use_Object_Table);
    return true;
  }
  
  return verify_all_free_lists()  &&  verify_all_segments(false);
}

bool Segmented_Object_Table::verify_all_free_lists() {
  FOR_ALL_RANKS(r) verify_free_list(r);
  return true;
}


bool Segmented_Object_Table::verify_free_list(int rank) {
  bool ok = true;
  for (Entry* e = first_free_entry[rank];  e != NULL;  e = e->word()->get_entry())
    ok = e->verify_free_entry((Object_Table*)this) && ok;
  return ok;
}


bool Segmented_Object_Table::verify_all_segments(bool live_ones_are_marked) {
  bool ok = true;
  FOR_ALL_RANKS(r)
  for (Segment* p = first_segment[r];  p != NULL;  p = p->next())
    ok = p->verify((Object_Table*)this, live_ones_are_marked) && ok;
  return ok;
}


bool Segmented_Object_Table::Segment::verify(Object_Table* ot, bool live_ones_are_marked) {
  bool ok = true;
  for ( Segmented_Object_Table::Entry* e = first_entry();  e < end_entry();  e = e->next() )
    ok = e->verify(ot, live_ones_are_marked) && ok;
  return ok;
}

bool Segmented_Object_Table::Entry::verify(Object_Table* ot, bool live_ones_are_marked) {
  return is_free(ot) || verify_used_entry(live_ones_are_marked);
}


// Slow; only for verification
bool Segmented_Object_Table::Entry::is_free(Object_Table* ot) {
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

bool Segmented_Object_Table::Entry::verify_free_entry(Object_Table* ot) {
  assert_always(is_free(ot));
  return true;
}

bool Segmented_Object_Table::Entry::verify_used_entry(bool live_ones_are_marked) {
  Object* obj = word()->obj();
  if (obj != NULL) {
    assert_always(The_Memory_System()->contains(obj));
    assert_always(!obj->is_marked() || live_ones_are_marked);
  }
  else
    fatal("no addr");
  return true;
}



void Segmented_Object_Table::Segment::set_next(Segment* s  COMMA_DCL_ESB) {
  if (!ESB_OR_FALSE || !Segmented_Object_Table::replicate) h._next = s;
  else {
    The_Memory_System()->pre_cohere_object_table(&h, sizeof(h));
    h._next = s;
    The_Memory_System()->post_cohere_object_table(&h, sizeof(h));
  }
}

void Segmented_Object_Table::Segment::print() {
  lprintf("\tSegment: first_entry 0x%x, end_entry 0x%x\n", first_entry(), end_entry());
}

static const char seg_cm[4] = "seg";

void Segmented_Object_Table::Segment::save_to_checkpoint(FILE* f, int rank) {
  Segment* me = this;
  write_mark(f, seg_cm);
  xfwrite(&rank, sizeof(rank), 1, f);
  xfwrite(&me, sizeof(me), 1, f);
  xfwrite(this, sizeof(*this), 1, f);
  // fprintf(stderr, "wrote a segment for %d at 0x%x, file is at 0x%x\n", rank, this, ftell(f));
}


void Segmented_Object_Table::Segment::restore_from_checkpoint(FILE* f, Segment* segs[], int n_segs) {
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


void Segmented_Object_Table::check_for_debugging(Oop x) {
  if (Use_Object_Table && ((Object_Table*)this)->probably_contains_not((void*)x.bits())) {
    lprintf("object_for caught one\n");
    fatal("caught it");
  }
}


