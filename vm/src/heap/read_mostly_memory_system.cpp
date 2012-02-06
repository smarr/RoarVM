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

u_int32  Read_Mostly_Memory_System::memory_per_read_mostly_heap = 0;
u_int32  Read_Mostly_Memory_System::log_memory_per_read_mostly_heap = 0;


void Read_Mostly_Memory_System::enforce_coherence_after_each_core_has_stored_into_its_own_heap() {
  if (!replicate_methods &&  !replicate_all) return;  // should not need this statement, but there was a bug without it, and anyway it's faster with it
  OS_Interface::mem_fence(); // ensure all cores see same heap _next's
  
  enforceCoherenceAfterEachCoreHasStoredIntoItsOwnHeapMessage_class().send_to_other_cores();
  
  heaps[Logical_Core::my_rank()][read_mostly]->enforce_coherence_in_whole_heap_after_store();
}

int Read_Mostly_Memory_System::calculate_total_read_mostly_pages(int page_size) {
  return divide_and_round_up(calculate_bytes_per_read_mostly_heap(page_size) * Logical_Core::group_size, page_size);
}


int Read_Mostly_Memory_System::calculate_bytes_per_read_mostly_heap(int /* page_size */) {
  int min_bytes_per_core = divide_and_round_up(min_heap_MB * Mega,  Logical_Core::group_size);
  return round_up_to_power_of_two(min_bytes_per_core);
}

void Read_Mostly_Memory_System::set_page_size_used_in_heap() {
  if (use_huge_pages) {
    int   co_pages = calculate_total_read_write_pages(huge_page_size);
    int inco_pages = calculate_total_read_mostly_pages(huge_page_size);
    if (!ask_Linux_for_huge_pages(co_pages + inco_pages))
      use_huge_pages = false;
  }
  lprintf("Using %s pages.\n", use_huge_pages ? "huge" : "normal");
  int hps = huge_page_size,  nps = normal_page_size; // compiler bug, need to alias these
  page_size_used_in_heap = use_huge_pages ? hps : nps;
}


void Read_Mostly_Memory_System::receive_heap(int i) {
  Logical_Core* sender;
  Multicore_Object_Heap** heaps_buf =
      (Multicore_Object_Heap**)Message_Queue::buffered_receive_from_anywhere(true, &sender, Logical_Core::my_core());
  heaps[i][read_mostly] = *heaps_buf;
  sender->message_queue.release_oldest_buffer(heaps_buf);
  Basic_Memory_System::receive_heap(i);
}

void Read_Mostly_Memory_System::initialize_main(init_buf* ib) {
  initialize_main_from_buffer((void*)ib, sizeof(*ib));
}

void Read_Mostly_Memory_System::initialize_from_snapshot(int32 snapshot_bytes, int32 sws, int32 fsf, int32 lastHash) {
  set_page_size_used_in_heap();
  
  int rw_pages = calculate_total_read_write_pages (page_size_used_in_heap);
  int rm_pages = calculate_total_read_mostly_pages(page_size_used_in_heap);
  // lprintf("rw_pages %d, rm_pages %d\n", rw_pages, rm_pages);
  
  
  snapshot_window_size.initialize(sws, fsf);
  
  
  u_int32 total_read_write_memory_size     =  rw_pages *  page_size_used_in_heap;
  u_int32 total_read_mostly_memory_size    =  rm_pages *  page_size_used_in_heap;
  
  read_mostly_memory_base = NULL;
  read_write_memory_base = NULL;
  
  map_read_write_and_read_mostly_memory(getpid(), total_read_write_memory_size, total_read_mostly_memory_size);
  
  memory_per_read_write_heap  = total_read_write_memory_size   / Logical_Core::group_size;
  memory_per_read_mostly_heap = calculate_bytes_per_read_mostly_heap(page_size_used_in_heap);
  
  assert(memory_per_read_write_heap                             <=  total_read_write_memory_size);
  assert(memory_per_read_write_heap * Logical_Core::group_size  <=  total_read_write_memory_size);
  
  assert(memory_per_read_mostly_heap                            <=  total_read_mostly_memory_size);
  assert(memory_per_read_mostly_heap * Logical_Core::group_size <=  total_read_mostly_memory_size);
  
  log_memory_per_read_write_heap = log_of_power_of_two(memory_per_read_write_heap);
  log_memory_per_read_mostly_heap = log_of_power_of_two(memory_per_read_mostly_heap);
  object_table = new Multicore_Object_Table();
  
  init_buf ib = {
    { snapshot_bytes, sws, fsf, lastHash,
    read_write_memory_base,
    total_read_write_memory_size, memory_per_read_write_heap, log_memory_per_read_write_heap,
    page_size_used_in_heap, getpid(),
    object_table,
    global_GC_values},
    read_mostly_memory_base,
    total_read_mostly_memory_size, memory_per_read_mostly_heap, log_memory_per_read_mostly_heap,
  };
  
  initialize_main(&ib);
}

/** The noinline attribute is necessary here to guarantee that LLVM-GCC,
 and Clang do not optimize the consistency checks at the point where this
 method is used.
 LLVM-GCC and Clang make the assumption that allocations are never as large
 as 2 GB and thus, convert the assertions to constant checks, which will
 fail for a 2 GB heap */
__attribute__((noinline))  // Important attribute for LLVM-GCC and Clang
void Read_Mostly_Memory_System::map_heap_memory_in_one_request(int pid,
                                                   size_t grand_total,
                                                   size_t inco_size,
                                                   size_t co_size) {
  read_mostly_memory_base = map_heap_memory(grand_total, grand_total,
                                            NULL, 0, pid, MAP_SHARED);
  read_mostly_memory_past_end = read_mostly_memory_base + inco_size;
  
  read_write_memory_base      = read_mostly_memory_past_end;
  read_write_memory_past_end  = read_write_memory_base + co_size;
}

/** Allocate heaps in two steps, to be able to set the flag for incoherent
 memory */
void Read_Mostly_Memory_System::map_heap_memory_separately(int pid,
                                               size_t grand_total,
                                               size_t inco_size,
                                               size_t co_size) {
  if (OS_mmaps_up) {
    read_mostly_memory_base     = map_heap_memory(grand_total, inco_size,
                                                  read_mostly_memory_base,
                                                  0, pid,
                                                  MAP_SHARED | MAP_CACHE_INCOHERENT);
    read_mostly_memory_past_end = read_mostly_memory_base + inco_size;
    
    read_write_memory_base      = map_heap_memory(grand_total, co_size,
                                                  read_mostly_memory_past_end,
                                                  inco_size, pid,
                                                  MAP_SHARED);
    read_write_memory_past_end  = read_write_memory_base + co_size;
  }
  else {
    read_write_memory_base      = map_heap_memory(grand_total, co_size,
                                                  read_write_memory_base,
                                                  0, pid,
                                                  MAP_SHARED);
    read_write_memory_past_end  = read_write_memory_base + co_size;
    
    read_mostly_memory_past_end = read_write_memory_base;
    read_mostly_memory_base     = map_heap_memory(grand_total, inco_size,
                                                  read_mostly_memory_past_end - inco_size,
                                                  co_size, pid,
                                                  MAP_SHARED | MAP_CACHE_INCOHERENT);
  }
}

void Read_Mostly_Memory_System::map_read_write_and_read_mostly_memory(int pid, size_t total_read_write_memory_size, size_t total_read_mostly_memory_size) {
  size_t     co_size = total_read_write_memory_size;
  size_t   inco_size = total_read_mostly_memory_size;
  size_t grand_total = co_size + inco_size;
  
  if (On_Tilera)
    map_heap_memory_separately(pid, grand_total, inco_size, co_size);
  else
    map_heap_memory_in_one_request(pid, grand_total, inco_size, co_size);
  
  assert(read_write_memory_base < read_write_memory_past_end);
  
  assert(read_mostly_memory_base < read_mostly_memory_past_end);
  assert(read_mostly_memory_past_end <= read_write_memory_base);
  
  if (read_mostly_memory_base >= read_write_memory_past_end) {
    unlink(mmap_filename);
    fatal("contains will fail");
  }
}

void Read_Mostly_Memory_System::send_local_heap() {
  Logical_Core::main_core()->message_queue.buffered_send_buffer(&heaps[Logical_Core::my_rank()][read_mostly], sizeof(Multicore_Object_Heap*));
  
  Basic_Memory_System::send_local_heap();
}

void Read_Mostly_Memory_System::map_memory_on_helper(init_buf* ib) {
  map_read_write_and_read_mostly_memory(ib->base_buf.main_pid,
                                        ib->base_buf.total_read_write_memory_size, 
                                        ib->total_read_mostly_memory_size);
}


void Read_Mostly_Memory_System::init_values_from_buffer(init_buf* ib) {
  memory_per_read_mostly_heap     = ib->memory_per_read_mostly_heap;
  log_memory_per_read_mostly_heap = ib->log_memory_per_read_mostly_heap;  
  read_mostly_memory_base         = ib->read_mostly_memory_base;

  Basic_Memory_System::init_values_from_buffer(&ib->base_buf);
}

void Read_Mostly_Memory_System::create_my_heaps(init_buf* ib) {
  const int my_rank = Logical_Core::my_rank();
  Basic_Memory_System::create_my_heaps(&ib->base_buf);
  
  Multicore_Object_Heap* h = new Multicore_Object_Heap();
  h->initialize_multicore(ib->base_buf.lastHash  +  Logical_Core::group_size + my_rank,
                          &read_mostly_memory_base[memory_per_read_mostly_heap * my_rank],
                          memory_per_read_mostly_heap,
                          page_size_used_in_heap,
                          false );
  heaps[my_rank][read_mostly] = h;
}

void Read_Mostly_Memory_System::scan_compact_or_make_free_objects_here(bool compacting, Abstract_Mark_Sweep_Collector* gc_or_null) {
  Basic_Memory_System::scan_compact_or_make_free_objects_here(compacting, gc_or_null);
  
  heaps[Logical_Core::my_rank()][read_mostly]->scan_compact_or_make_free_objects(compacting, gc_or_null);
}

void Read_Mostly_Memory_System::print() {
  lprintf("Read_Mostly_Memory_System:\n");
  lprintf("memory_per_read_mostly_heap 0x%x, log_memory_per_read_mostly_heap %d\n"
          "read_mostly_memory_base 0x%x, read_mostly_memory_past_end 0x%x, "
          "second_chance_cores_for_allocation[read_mostly] %d,\n",
          memory_per_read_mostly_heap, log_memory_per_read_mostly_heap,
          read_mostly_memory_base, read_mostly_memory_past_end,
          second_chance_cores_for_allocation[read_mostly]);

  Basic_Memory_System::print();
}

bool Read_Mostly_Memory_System::moveAllToRead_MostlyHeaps() {
  Safepoint_for_moving_objects sm("moveAllToRead_MostlyHeaps");
  Safepoint_Ability sa(false);
  
  flushFreeContextsMessage_class().send_to_all_cores();
  
  fullGC("moveAllToRead_MostlyHeaps");
  The_Squeak_Interpreter()->preGCAction_everywhere(false);  // false because caches are oop-based, and we just move objs
  u_int32 old_gcCount = global_GC_values->gcCount; // cannot tolerate GCs, ends gets messed up
  
  Timeout_Deferral td;
  
  FOR_ALL_RANKS(i) {
    Multicore_Object_Heap* h = heaps[i][read_write];
    for ( Object* obj = h->first_object_or_null(), *next = NULL;
         obj != NULL;
         obj = next ) {
      next = h->next_object(obj);
      if (obj->isFreeObject()  ||  !obj->is_suitable_for_replication())
        continue;
      
      for (int dst_rank = i, n = 0;
           n < Logical_Core::group_size;
           ++n, ++dst_rank, dst_rank %= Logical_Core::group_size)  {
        
        if (n == Logical_Core::group_size) {
          lprintf("moveAllToRead_MostlyHeaps failing; out of space\n", i);
          return false;
        }
        
        if (u_int32(obj->sizeBits() + 32 + heaps[dst_rank][read_mostly]->lowSpaceThreshold)  >  heaps[dst_rank][read_mostly]->bytesLeft(false))
          continue;
        
        obj->move_to_heap(dst_rank, read_mostly, false);
        if (global_GC_values->gcCount != old_gcCount) {
          The_Squeak_Interpreter()->postGCAction_everywhere(false);
          lprintf("moveAllToRead_MostlyHeaps failing for core %d; GCed\n", i);
          return false;
        }
        break;
      }
    }
    fprintf(stderr, "finished rank %d\n", i);
  }
  The_Squeak_Interpreter()->postGCAction_everywhere(false);
  return true;
}

void Read_Mostly_Memory_System::push_heap_stats() {
  Oop readWriteHeapStats = The_Memory_System()->heaps[Logical_Core::my_rank()][read_write]->get_stats();
  PUSH_WITH_STRING_FOR_MAKE_ARRAY(readWriteHeapStats);

  Oop readMostlyHeapStats = The_Memory_System()->heaps[Logical_Core::my_rank()][read_mostly]->get_stats();
  PUSH_WITH_STRING_FOR_MAKE_ARRAY(readMostlyHeapStats);
  
}


