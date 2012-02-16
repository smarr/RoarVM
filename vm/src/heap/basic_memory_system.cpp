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

void Basic_Memory_System::push_heap_stats() {
  Oop readWriteHeapStats = The_Memory_System()->heaps[Logical_Core::my_rank()][read_write]->get_stats();
  PUSH_WITH_STRING_FOR_MAKE_ARRAY(readWriteHeapStats);
  
  /* STEFAN: I give here the read-write stats again, to avoid the
             need for adapting the image.
             That's the lazy solution, sorry. */
  Oop readMostlyHeapStats = The_Memory_System()->heaps[Logical_Core::my_rank()][read_write]->get_stats();
  PUSH_WITH_STRING_FOR_MAKE_ARRAY(readMostlyHeapStats);
}


void Basic_Memory_System::initialize_from_snapshot(int32 snapshot_bytes, int32 sws, int32 fsf, int32 lastHash) {
  set_page_size_used_in_heap();

  int rw_pages = calculate_pages_for_segmented_heap(page_size_used_in_heap);
  // lprintf("rw_pages %d, rm_pages %d\n", rw_pages, rm_pages);


  snapshot_window_size.initialize(sws, fsf);


  u_int32 total_read_write_memory_size =  rw_pages *  page_size_used_in_heap;

  read_write_memory_base = NULL;

  map_heap_memory_in_one_request(getpid(), total_read_write_memory_size, read_write_memory_base);

  memory_per_read_write_heap  = total_read_write_memory_size   / Logical_Core::group_size;
  
  assert(memory_per_read_write_heap                             <=  total_read_write_memory_size);
  assert(memory_per_read_write_heap * Logical_Core::group_size  <=  total_read_write_memory_size);

  
  log_memory_per_read_write_heap = log_of_power_of_two(memory_per_read_write_heap);
  object_table = new Multicore_Object_Table();

  init_buf ib = {
    snapshot_bytes, sws, fsf, lastHash,
    read_write_memory_base,
    total_read_write_memory_size, memory_per_read_write_heap, log_memory_per_read_write_heap,
    page_size_used_in_heap, getpid(),
    object_table,
    global_GC_values
  };

  initialize_main(&ib);
}


void Basic_Memory_System::set_page_size_used_in_heap() {
  if (use_huge_pages) {
    int   co_pages = calculate_pages_for_segmented_heap(huge_page_size);
    if (!OS_Interface::ask_for_huge_pages(co_pages))
      use_huge_pages = false;
  }
  lprintf("Using %s pages.\n", use_huge_pages ? "huge" : "normal");
  int hps = huge_page_size,  nps = normal_page_size; // compiler bug, need to alias these
  page_size_used_in_heap = use_huge_pages ? hps : nps;
}


/** The noinline attribute is necessary here to guarantee that LLVM-GCC,
    and Clang do not optimize the consistency checks at the point where this
    method is used.
    LLVM-GCC and Clang make the assumption that allocations are never as large
    as 2 GB and thus, convert the assertions to constant checks, which will
    fail for a 2 GB heap */
__attribute__((noinline))  // Important attribute for LLVM-GCC and Clang
void Basic_Memory_System::map_heap_memory_in_one_request(int pid, size_t total, void* start_address) {
  read_write_memory_base = OS_Interface::map_heap_memory(total, total,
                                                         start_address, 0, pid, MAP_SHARED);
  read_write_memory_past_end = read_write_memory_base + total;
  assert(read_write_memory_base < read_write_memory_past_end);
}

void Basic_Memory_System::receive_heap(int i) {
  Logical_Core* sender;
  Multicore_Object_Heap** heaps_buf =
      (Multicore_Object_Heap**)Message_Queue::buffered_receive_from_anywhere(true, &sender, Logical_Core::my_core());
  heaps[i][read_write ] = *heaps_buf;
  sender->message_queue.release_oldest_buffer(heaps_buf);
}

void Basic_Memory_System::initialize_main(init_buf* ib) {
  THIS->initialize_main_from_buffer(ib, sizeof(*ib));
}


void Basic_Memory_System::map_memory_on_helper(init_buf* ib) {
  map_heap_memory_in_one_request(ib->main_pid,
                                 ib->total_read_write_memory_size,
                                 ib->read_write_memory_base);
}

void Basic_Memory_System::send_local_heap() {
  Logical_Core::main_core()->message_queue.buffered_send_buffer(&heaps[Logical_Core::my_rank()][read_write ], sizeof(Multicore_Object_Heap*));
  
  if (check_many_assertions) lprintf("finished sending my heaps\n");
}


void Basic_Memory_System::init_values_from_buffer(init_buf* ib) {
  memory_per_read_write_heap     = ib->memory_per_read_write_heap;
  log_memory_per_read_write_heap = ib->log_memory_per_read_write_heap;
  
  page_size_used_in_heap = ib->page_size;
  
  read_write_memory_base = ib->read_write_memory_base;

  object_table = ib->object_table;

  snapshot_window_size.initialize(ib->sws, ib->fsf);

  global_GC_values = ib->global_GC_values;
}


// memory system is private; but heaps is shared

void Basic_Memory_System::create_my_heaps(init_buf* ib) {
  const int my_rank = Logical_Core::my_rank();

  Multicore_Object_Heap* h = new Multicore_Object_Heap();
  h->initialize_multicore( ib->lastHash + my_rank,
                 &read_write_memory_base[memory_per_read_write_heap * my_rank],
                 memory_per_read_write_heap,
                 page_size_used_in_heap,
                 On_Tilera );
  heaps[my_rank][read_write] = h;
}


void Basic_Memory_System::scan_compact_or_make_free_objects_here(bool compacting, Abstract_Mark_Sweep_Collector* gc_or_null) {
  heaps[Logical_Core::my_rank()][read_write ]->scan_compact_or_make_free_objects(compacting, gc_or_null);
}


void Basic_Memory_System::print() {
  lprintf("Memory_System:\n");
  lprintf("use_huge_pages: %d, min_heap_MB %d, replicate_methods %d, replicate_all %d, memory_per_read_write_heap 0x%x, log_memory_per_read_write_heap %d,\n"
                  "read_write_memory_base 0x%x, read_write_memory_past_end 0x%x, "
                  "page_size_used_in_heap %d, round_robin_period %d, second_chance_cores_for_allocation[read_write] %d,\n"
                  "gcCount %d, gcMilliseconds %d, gcCycles %lld\n",
                  use_huge_pages, min_heap_MB, replicate_methods, replicate_all, memory_per_read_write_heap, log_memory_per_read_write_heap,
                  read_write_memory_base, read_write_memory_past_end,
                  page_size_used_in_heap, round_robin_period, 
                  second_chance_cores_for_allocation[read_write], 
                  global_GC_values->gcCount, global_GC_values->gcMilliseconds, global_GC_values->gcCycles);
  if ( object_table != NULL )
    object_table->print();

  THIS->print_heaps();
}

