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


# if On_Tilera_With_GCC

#include "headers.h"

/*
 STEFAN: Discuss with david why he needed uncacheable here 
 
 static TMC_OS_Interface::OS_Heap os_heap;
static bool created = false;

void* TMC_OS_Interface::malloc_uncacheable_shared(int alignment, int size) {
  if (alignment == 0)
    alignment = ilib_mem_get_cacheline_size();
  
  if (!created) {
    int err = ilib_mem_create_heap(ILIB_MEM_UNCACHEABLE | ILIB_MEM_SHARED,
                                   &us_heap);
    abort_if_error("malloc_uncacheable_shared", err);
    created = true;
  }
  void* r = ilib_mem_memalign_heap(us_heap, alignment, size);
  if (r == NULL)
    fatal("malloc_uncacheable_shared");
  return r;
}*/



void TMC_OS_Interface::start_processes(void (*helper_core_main)(), char* argv[]) {
  // go parallel; one core returns; others run helper_core_main fn
  
  # warning STEFAN: refactor, add a setter method for initializing those values.
  Logical_Core::remaining = ilib_proc_remaining();
  Logical_Core::group_size = ilib_group_size(ILIB_GROUP_SIBLINGS);
  Memory_Semantics::_my_rank = ilib_group_rank(ILIB_GROUP_SIBLINGS);
  Memory_Semantics::_my_rank_mask = 1LL << u_int64(Memory_Semantics::_my_rank);
  CPU_Coordinate::_my_x = udn_tile_coord_x();
  CPU_Coordinate::_my_y = udn_tile_coord_y();
  
  if (Logical_Core::group_size == 1  &&  Logical_Core::group_size < Logical_Core::num_cores) {
    ilibProcParam params;
    memset(&params, 0, sizeof(params));
    params.num_procs = Logical_Core::num_cores;
    params.binary_name = NULL;
    params.argv = argv;
    
    params.tiles.x      = params.tiles.y = 0;
    params.tiles.width  = CPU_Coordinate::width;
    params.tiles.height = CPU_Coordinate::height;
    
    // skip params.init_block/size

    lprintf("Will ask for num_proc: %d on w:%d;h:%d\n", params.num_procs, params.tiles.width, params.tiles.height);
    
    int err = ilib_proc_exec(1, &params);
    abort_if_error("exec", err);
    die("impossible");
  }
  
  Logical_Core::initialize_all_cores();
  Memory_Semantics::_my_core = &logical_cores[Memory_Semantics::_my_rank];
  
  Memory_Semantics::initialize_interpreter();
  Memory_Semantics::initialize_local_interpreter();
  
  ILib_Message_Queue::setup_channels();
  
  if (Measure_Communication)
    Logical_Core::my_core()->message_queue.measure_communication();
  
  // lprintf("is_center: %s, center_rank: %d, main_rank: %d, my_rank: %d\n",
  //         CPU_Coordinate::is_center() ? "true" : "false",
  //         CPU_Coordinate::center_rank,
  //         Logical_Core::main_rank,
  //         Logical_Core::my_rank());
  //
  // lprintf("center_x: %d, center_y: %d, my_x: %d, my_y: %d\n",
  //         Tile_CPU_Coordinate::center_x,
  //         Tile_CPU_Coordinate::center_y,
  //         Tile_CPU_Coordinate::_my_x,
  //         Tile_CPU_Coordinate::_my_y);
  
  if (CPU_Coordinate::is_center() != (CPU_Coordinate::center_rank == Logical_Core::my_rank()))
    fatal("center_rank is wrong\n");
  
  if (Logical_Core::running_on_main()) {
    fprintf(stdout, "spawned %d helpers\n", Logical_Core::group_size - 1);
    return;
  }
  else {
    (*helper_core_main)();
    char buf[BUFSIZ];
    Logical_Core::my_print_string(buf, sizeof(buf));
    lprintf( "helper finsihed: %s\n", buf);
    rvm_exit();
  }
  
}


int TMC_OS_Interface::abort_if_error(const char* msg, int err) {
  if (err >= 0)  return err;
  lprintf( "%s failed: %s\n", msg, ilib_debug_strerror(err));
  abort();
  return 0;
}




# endif // On_Tilera
