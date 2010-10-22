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


void* Multicore_Object_Heap::operator new(size_t size) {
  return Memory_Semantics::shared_calloc(1, size);
}


void Multicore_Object_Heap::initialize_multicore(int hash, char* mem, int size, int page_size, bool do_homing) {
  Abstract_Object_Heap::initialize(mem,size);
  lastHash = hash;
  if (do_homing  &&  Logical_Core::group_size > 1)
    home_to_this_tile(page_size);
}

void Multicore_Object_Heap::home_to_this_tile(int page_size) {
  for (char* p = (char*)_start;  p < (char*)_end;  p += page_size)
    *p = '\xff';
  if (!The_Squeak_Interpreter()->use_checkpoint()) // save time
    verify_homing(page_size);
}

int a_global;

bool Multicore_Object_Heap::verify_homing(int page_size) {
  // lprintf("beginning homing test...\n");
  OS_Interface::get_cycle_count_quickly_t baseline = 1000000;
  OS_Interface::get_cycle_count_quickly_t min_read_time = 1000000, max_read_time = 0;
  int min_time_page = -1, max_time_page = -1;
  const int N = 10;
  for (int i = 0;  i < N;  ++i) {
    OS_Interface::get_cycle_count_quickly_t start = GET_CYCLE_COUNT_QUICKLY();
    a_global = i;
    OS_Interface::get_cycle_count_quickly_t end = GET_CYCLE_COUNT_QUICKLY();
    OS_Interface::get_cycle_count_quickly_t d = end - start;
    if (d > 0  &&  d  <  baseline)  baseline = d;
  }
  for (char* p = (char*)_start;  p < (char*)_end;  p += page_size) {
    OS_Interface::get_cycle_count_quickly_t min_this_page = 10000;
    for (int i = 0;  i < N;  i++) {
      OS_Interface::get_cycle_count_quickly_t start = GET_CYCLE_COUNT_QUICKLY();
      a_global = *p;
      OS_Interface::get_cycle_count_quickly_t end = GET_CYCLE_COUNT_QUICKLY();
      OS_Interface::get_cycle_count_quickly_t d = end - start;
      if (d > 1000)  continue;
      if (d < min_this_page) min_this_page = d;
    }
    if (min_this_page  <  min_read_time)  { min_read_time = min_this_page;  min_time_page = (p - (char*)_start) / page_size; }
    if (min_this_page  >  max_read_time)  { max_read_time = min_this_page;  max_time_page = (p - (char*)_start) / page_size; }
  }
  min_read_time -= baseline;  max_read_time -= baseline;
  // lprintf("finishing homing test...\n");
  if (max_read_time <= 5  ||  /* might be neg, but is unsigned*/ max_read_time + 10  <=  10)
    return true;
  lprintf("read_time = " GET_CYCLE_COUNT_QUICKLY_FMT "(page %d)-" GET_CYCLE_COUNT_QUICKLY_FMT "(page %d)\n",
          min_read_time, min_time_page, max_read_time, max_time_page);
  fatal("homing");
  return false;
}


void Multicore_Object_Heap::add_object_from_snapshot(Oop dst_oop, Object* dst_obj, Object* src_obj_wo_preheader) {
  int extra_header_oops_wo_preheader = src_obj_wo_preheader->extra_header_oops_without_preheader();
  Oop* dst_chunk_wo_preheader = &dst_obj         ->as_oop_p()[-extra_header_oops_wo_preheader];
  Oop* src_chunk_wo_preheader = &src_obj_wo_preheader->as_oop_p()[-extra_header_oops_wo_preheader];

  int total_src_bytes = src_obj_wo_preheader->total_byte_size_without_preheader();


  // avoid store barrier; do it in caller:
  IMAGE_READING_DEBUG_MULTIMOVE_CHECK(dst_chunk_wo_preheader, src_chunk_wo_preheader, total_src_bytes / bytes_per_oop);
  memcpy(dst_chunk_wo_preheader,  src_chunk_wo_preheader,  total_src_bytes);
  // beRootIfOld -- What to do about old-young barrier?

  dst_obj->set_preheader(dst_oop); // now that baseHeader is set, can do this
}




void Multicore_Object_Heap::flushExternalPrimitives() {
  FOR_EACH_OBJECT_IN_HEAP(this, oop) {
    if (!oop->isFreeObject()
        &&   oop->isCompiledMethod()
        &&   oop->primitiveIndex() == Squeak_Interpreter::PrimitiveExternalCallIndex)
      oop->flushExternalPrimitive();
  }
}


void Multicore_Object_Heap::handle_low_space_signal() {
  if (The_Squeak_Interpreter()->signalLowSpace())  {
    The_Squeak_Interpreter()->set_signalLowSpace(false);
    The_Squeak_Interpreter()->signalSema(Special_Indices::TheLowSpaceSemaphore, "handle_low_space_signal");
  }
}



Oop Multicore_Object_Heap::next_instance_of_after(Oop klass, Oop x) {
  for (Object* r = x.as_object();  ;  ) {
    r = accessibleObjectAfter(r);
    if (r == NULL)
      return The_Squeak_Interpreter()->roots.nilObj;
    if (r->fetchClass() == klass)
      return r->as_oop();
  }
}



void Multicore_Object_Heap::snapshotCleanUp() {
  FOR_EACH_OBJECT_IN_HEAP(this, obj) {
    if (obj->isFreeObject())
      continue;
    if (Object::Format::might_be_context(obj->format()) && obj->hasContextHeader()) {
      int bytes_to_last_pointer = obj->lastPointer();
      int bytes_to_pointer_after_last = bytes_to_last_pointer + sizeof(Oop);
      int total_bytes = obj->sizeBits();
      int oops_to_zap = (total_bytes - bytes_to_pointer_after_last) >> ShiftForWord;
      Oop* zap_start = obj->as_oop_p() + (bytes_to_pointer_after_last >> ShiftForWord);
      assert(The_Memory_System()->contains(zap_start));
      oopset_no_store_check(
                            zap_start,
                            The_Squeak_Interpreter()->roots.nilObj,
                            oops_to_zap);
    }
    else if (Object::Format::isCompiledMethod(obj->format())
             &&  obj->primitiveIndex() == Squeak_Interpreter::PrimitiveExternalCallIndex)
      obj->flushExternalPrimitive();
  }
}



void Multicore_Object_Heap::write_image_file(FILE* f, u_int32* address_offsets, bool& is_first_object) {
  

  const oop_int_t preheader_placeholder = Object::make_free_object_header(preheader_byte_size);
  __attribute__((unused)) Object* last_obj = NULL; // debugging aid
  
  // Squeak 64-bit VM bug workaround
  // 64-bit Squeak VM starts oops at zero and uses squeakMemoryBase
  // But it uses nil tests in for firstFree and freeChunk in the sweep phase
  // If after it does a GC, the first word is free, "sweepPhase" fails!!! -- dmu 6/13/10
  // So, we need to not output those first free words. Since our snapshot just did a GC, this should be only the preheader. -- dmu 6/10
  
  
  FOR_EACH_OBJECT_IN_HEAP(this, obj) {
    if (obj->isFreeObject()) {
      assert_always_msg(!is_first_object, "first object must not be free to work with Squeak 64 bit VM"); // Squeak 64-bit VM bug workaround
      int bytes = obj->sizeOfFree();
      oop_int_t* p = obj->as_oop_int_p();
      for (int i = 0;  i < bytes;  i += sizeof(int32))
        The_Memory_System()->putLong(*p++, f);
      last_obj = obj;
      continue;
    }

    assert(sizeof(long) == sizeof(Oop));
    if (preheader_oop_size  &&  !is_first_object /* see long comment above */) { // Squeak 64-bit VM bug workaround
      The_Memory_System()->putLong(preheader_placeholder, f);
      for (int i = 1;  i  <  preheader_oop_size;  ++i)
        The_Memory_System()->putLong(Oop::Illegals::free_extra_preheader_words, f);
    }

    if (obj->contains_sizeHeader())
      The_Memory_System()->putLong(obj->sizeHeader(), f);

    if (obj->contains_class_and_type_word())
      The_Memory_System()->putLong(
              Header_Type::extract_from( obj->class_and_type_word() )
              |  Header_Type::without_type( The_Memory_System()->adjust_for_snapshot(obj->get_class_oop().as_object(), address_offsets) ),
              f);
    The_Memory_System()->putLong(obj->baseHeader, f);

    oop_int_t* p;
    for ( p = &obj->baseHeader + 1;
         (Oop*)p <= obj->last_pointer_addr();
         ++p ) {
       Oop oop = *(Oop*)p;
       The_Memory_System()->putLong( oop.is_int()
            ? oop.bits()
            : The_Memory_System()->adjust_for_snapshot(oop.as_object(), address_offsets),  f);
    }
    for (Chunk* next = obj->nextChunk();  p < (oop_int_t*)next;  The_Memory_System()->putLong(*p++, f))
      ; // bytes
    last_obj = obj;
    
    is_first_object = false;
  }
}

Oop Multicore_Object_Heap::get_stats() {
  int s = The_Squeak_Interpreter()->makeArrayStart();
  PUSH_POSITIVE_32_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(bytesUsed());
  PUSH_POSITIVE_32_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(bytesLeft());
  PUSH_POSITIVE_32_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(allocationsSinceLastQuery);
  PUSH_POSITIVE_32_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(compactionsSinceLastQuery);
  allocationsSinceLastQuery = compactionsSinceLastQuery = 0;
  return The_Squeak_Interpreter()->makeArray(s);
}


static const char check_mark[4] = "moh";


void Multicore_Object_Heap::save_to_checkpoint(FILE* f) {
  write_mark(f, check_mark);
  xfwrite(this, sizeof(*this), 1, f);
  xfwrite(_start, sizeof(*_start), _next - _start, f);
}

void Multicore_Object_Heap::restore_from_checkpoint(FILE* f) {
  lprintf( "restoring a heap...\n");
  read_mark(f, check_mark);

  Multicore_Object_Heap lcl;
  xfread(&lcl, sizeof(lcl), 1, f);

  lastHash = lcl.lastHash;
  if (_start != lcl._start) fatal("_start mismatch");
  _next = lcl._next;
  if (_end != lcl._end) fatal("_end mismatch");
  xfread(_start, sizeof(*_start), lcl._next - lcl._start, f);

  allocationsSinceLastQuery = lcl.allocationsSinceLastQuery;
  compactionsSinceLastQuery = lcl.compactionsSinceLastQuery;
  lowSpaceThreshold = lcl.lowSpaceThreshold;
}


void Multicore_Object_Heap::print(FILE* f) {
  Abstract_Object_Heap::print(f);
}

