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

# define THIS ((Memory_System*)this)

bool     Abstract_Memory_System::use_huge_pages = On_Tilera;
bool     Abstract_Memory_System::replicate_methods = false; // if true methods are put on read-mostly heap
bool     Abstract_Memory_System::replicate_all = true; // if true, all (non-contexts) are allowed in read-mostly heap
bool     Abstract_Memory_System::OS_mmaps_up = On_Apple;
u_int32  Abstract_Memory_System::memory_per_read_write_heap = 0;
u_int32  Abstract_Memory_System::log_memory_per_read_write_heap = 0;
int      Abstract_Memory_System::round_robin_period = 1;
size_t   Abstract_Memory_System::min_heap_MB =  On_iOS ? 32 : On_Tilera ? 256 : 1024; // Fewer GCs on Mac

Abstract_Memory_System::Abstract_Memory_System() {
  image_name = new char[1]; *image_name = '\0';
  
  global_GC_values = (struct Global_GC_Values*)Memory_Semantics::shared_malloc(sizeof(struct Global_GC_Values));
  global_GC_values->growHeadroom = 4 * Mega;
  global_GC_values->shrinkThreshold = 8 * Mega;
  global_GC_values->gcCycles = 0;
  global_GC_values->gcCount = 0;
  global_GC_values->gcMilliseconds = 0;
  global_GC_values->mutator_start_time = 0;
  global_GC_values->last_gc_ms = 0;
  global_GC_values->inter_gc_ms = 0;
  
  page_size_used_in_heap = 0;
}

void Abstract_Memory_System::imageNamePut_on_this_core(const char* n, int len) {
  delete[] image_name;
  image_name = new char[len + 1];
  bcopy(n, image_name, len);
  image_name[len] = '\0';
}

char* Abstract_Memory_System::imageName() { return image_name; }
int   Abstract_Memory_System::imageNameSize() { return strlen(image_name); }

int Abstract_Memory_System::calculate_pages_for_segmented_heap(int page_size) {
  int min_heap_bytes_for_all_cores = min_heap_MB * Mega;
  int min_heap_bytes_per_core = divide_and_round_up(min_heap_bytes_for_all_cores, Logical_Core::group_size);
  int min_pages_per_core = divide_and_round_up(min_heap_bytes_per_core, page_size);
  int pages_per_core = round_up_to_power_of_two(min_pages_per_core); // necessary so per-core bytes is power of two

  return pages_per_core * Logical_Core::group_size;
}

int Abstract_Memory_System::round_robin_rank() {
  assert(Logical_Core::running_on_main());
  static int i = 0; // threadsafe? think its ok, there is no need for 100% monotony, Stefan, 2009-09-05
  return i++ % Logical_Core::group_size;
}


int Abstract_Memory_System::assign_rank_for_snapshot_object() {
  return round_robin_rank();
}


void Abstract_Memory_System::swapOTEs(Oop* o1, Oop* o2, int len) {
  for (int i = 0;  i < len;  ++i) {
    Object_p obj1 = o1[i].as_object();
    Object_p obj2 = o2[i].as_object();
    
    obj2->set_object_address_and_backpointer(o1[i]  COMMA_TRUE_OR_NOTHING);
    obj1->set_object_address_and_backpointer(o2[i]  COMMA_TRUE_OR_NOTHING);
  }
}

void Abstract_Memory_System::writeImageFile(char* image_name) {
  THIS->writeImageFileIO(image_name);
  fn_t setMacType = The_Interactions.load_function_from_plugin(Logical_Core::main_rank, "setMacFileTypeAndCreator", "FilePlugin");
  if (setMacType != NULL)  (*setMacType)(The_Memory_System()->imageName(), "STim", "FAST");
}



