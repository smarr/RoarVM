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

Interactions The_Interactions;


Object_p last_ctx_rcv; // xxx for debugging


// me or one other:

fn_t Interactions::load_function_from_plugin(int dst, const char* fn_name, const char* plugin) {
  const size_t rank_on_threads_or_zero_on_processes = Memory_Semantics::rank_on_threads_or_zero_on_processes();

  if (dst == Logical_Core::my_rank()) return ioLoadFunctionFrom(fn_name, plugin);

  static char* fn_name_buf     = (char*)Memory_Semantics::shared_malloc(BUFSIZ * Memory_Semantics::max_num_threads_on_threads_or_1_on_processes);  // read as fn_name_buf[Memory_Semantics::max_num_threads_on_threads_or_1_on_processes][BUFSIZ]     // threadsafe
  static char* plugin_name_buf = (char*)Memory_Semantics::shared_malloc(BUFSIZ * Memory_Semantics::max_num_threads_on_threads_or_1_on_processes);  // read as plugin_name_buf[Memory_Semantics::max_num_threads_on_threads_or_1_on_processes][BUFSIZ] // threadsafe

  char* local_fn_name_buf     = fn_name_buf     + (rank_on_threads_or_zero_on_processes * BUFSIZ);  // read as fn_name_buf[rank_on_threads_or_zero_on_processes]
  char* local_plugin_name_buf = plugin_name_buf + (rank_on_threads_or_zero_on_processes * BUFSIZ);  // read as plugin_name_buf[rank_on_threads_or_zero_on_processes]

  snprintf(local_fn_name_buf, BUFSIZ, "%s", fn_name);
  snprintf(local_plugin_name_buf, BUFSIZ, "%s", plugin);
  
  SEND_THEN_WAIT_AND_RETURN_MESSAGE( loadFunctionFromPluginMessage_class(local_fn_name_buf, local_plugin_name_buf), dst,
                                     loadFunctionFromPluginResponse, r);

  return r.fn;
}


void Interactions::get_screen_info(int* screenSize, int* fullScreenFlag) {
  if (Logical_Core::running_on_main()) {
    *screenSize = ioScreenSize();
    *fullScreenFlag = The_Memory_System()->snapshot_window_size.fullScreenFlag();
  }
  else {
    SEND_THEN_WAIT_AND_RETURN_MESSAGE(screenInfoMessage_class(), Logical_Core::main_rank, screenInfoResponse, r);
    *screenSize = r.screenSize;
    *fullScreenFlag = r.fullScreenFlag;
  }
}


bool Interactions::getNextEvent_on_main(int* evtBuf) {
  if (Logical_Core::running_on_main())
      return The_Squeak_Interpreter()->getNextEvent_any_platform(evtBuf);

  SEND_THEN_WAIT_AND_RETURN_MESSAGE(getNextEventMessage_class(), Logical_Core::main_rank, getNextEventResponse, r);
  if (!r.got_one) return false;
  for (int i = 0;  i < evtBuf_size;  ++i) evtBuf[i] = r.evtBuf[i];
  return true;
}


// return dst_obj so this tile can set OTE since OT can now be in read_mostly memory
Object* Interactions::add_object_from_snapshot_allocating_chunk(int dst, Oop dst_oop, Object* src_obj_wo_preheader) {
  if (dst == Logical_Core::my_rank()) {
    return The_Memory_System()->add_object_from_snapshot_to_a_local_heap_allocating_chunk(dst_oop, src_obj_wo_preheader);
  }
  const bool verbose = false;
  if (verbose)
    lprintf("sending add_object_from_snapshot_allocating_chunk to %d, dst 0x%x, src 0x%x\n", dst, dst_oop.bits(), src_obj_wo_preheader);

  SEND_THEN_WAIT_AND_RETURN_MESSAGE(addObjectFromSnapshotMessage_class(dst_oop, src_obj_wo_preheader), dst,
                                    addObjectFromSnapshotResponse, r);
  if (verbose)
    lprintf("returned add_object_from_snapshot_allocating_chunk from %d, dst 0x%x\n", dst, r.dst_obj);

  return r.dst_obj;
}



static void run_primitive_print(fn_t f, const char* long_msg, const char* short_msg) {
  static const bool verbose = false;
  if (!verbose) return;
  if (The_Squeak_Interpreter()->get_global_sequence_number() > 20)
    fprintf(stderr, "%s", short_msg);
  else
    lprintf("%s send of a primitive 0x%x %d\n", long_msg, f,
            The_Squeak_Interpreter()->increment_global_sequence_number());
}

void Interactions::run_primitive(int dst, fn_t f) {
  Message_Statics::remote_prim_fn = f;

  ++remote_prim_count;
  u_int64 start = OS_Interface::get_cycle_count();

  if (dst == Logical_Core::my_rank()) {
    f();
    remote_prim_cycles += OS_Interface::get_cycle_count() - start;
    return;
  }
  run_primitive_print(f, "caught", "<");
  
  SEND_THEN_WAIT_FOR_MESSAGE( runPrimitiveMessage_class(The_Squeak_Interpreter()->get_argumentCount(), f), dst,
                              runPrimitiveResponse);
  
  remote_prim_cycles += OS_Interface::get_cycle_count() - start;

  run_primitive_print(f, "returned from", ">");

  Message_Statics::remote_prim_fn = 0;
}





//   with waiting one by one

Oop Interactions::sample_each_core(int what_to_sample) {
  int s = The_Squeak_Interpreter()->makeArrayStart();
  
  sampleOneCoreMessage_class m(what_to_sample);
  FOR_ALL_RANKS(i) {
    Oop cpuCoreStats;
    if (i == Logical_Core::my_rank())  {
      cpuCoreStats = sample_one_core(what_to_sample);
    }
    else {
      SEND_THEN_WAIT_AND_RETURN_MESSAGE(m, i, sampleOneCoreResponse, m2);
      Oop preserved = The_Squeak_Interpreter()->popRemappableOop();
      cpuCoreStats = m2.result;
      
      // Commenting out the next line and using preserved in the line below are temporary workarounds.
      // The real cause of the assertion failure needs to be found. -- dmu 10/10
      // TODO: identify the problem leading to the case that the following
      //       assertion does not hold
      // assert_always(preserved == cpuCoreStats);
      if (preserved != cpuCoreStats)
        lprintf("Warning: patched around potential bug in interactions.cpp\n");

      // TODO: remove this workaround after identifying the bug causing the assertion to fail
      cpuCoreStats = preserved;
    }
    assert_always(cpuCoreStats.fetchClass() == The_Squeak_Interpreter()->splObj(Special_Indices::ClassArray)); // in case a bug returns
    PUSH_FOR_MAKE_ARRAY(cpuCoreStats);
  }
  return The_Squeak_Interpreter()->makeArray(s);
}



void Interactions::do_all_roots_here(Oop_Closure* oc) {
  doAllRootsHereMessage_class m(oc, false); // this closure will run HERE, and no GCs permitted when other side is reported back roots

  FOR_ALL_RANKS(i)
    if (i != Logical_Core::my_rank()) {
      SEND_THEN_WAIT_FOR_MESSAGE(m, i, noMoreRootsResponse);
    }
    else
      The_Squeak_Interpreter()->do_all_roots(oc);
}







Oop Interactions::get_stats() {
  int s = The_Squeak_Interpreter()->makeArrayStart();
  PUSH_POSITIVE_32_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(remote_prim_count );  remote_prim_count  = 0;
  PUSH_POSITIVE_64_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(remote_prim_cycles);  remote_prim_cycles = 0LL;
  return The_Squeak_Interpreter()->makeArray(s);
}

