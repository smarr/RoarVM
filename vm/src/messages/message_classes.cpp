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

void updateEnoughInterpreterToTransferControlMessage_class::send_to(int r) {
  subset.set_from_interpreter(); //include order, sigh

  The_Squeak_Interpreter()->storeContextRegisters(The_Squeak_Interpreter()->activeContext_obj()); // added for the invarients xxxxxx rm when debugged?
  The_Squeak_Interpreter()->set_activeContext(The_Squeak_Interpreter()->roots.nilObj); // so this core won't try to update active context when GC happens
  The_Squeak_Interpreter()->multicore_interrupt_check = true; // go into multicore_interrupt to wait for a process to execute; probably not really needed, since interp should get restored before top of interp loop
  The_Squeak_Interpreter()->set_running_process(The_Squeak_Interpreter()->roots.nilObj, "send_for_control_transfer"); // prevent setting saved CTX to nil in the process

  abstractMessage_class::send_to(r);
}




void  aboutToWriteReadMostlyMemoryMessage_class::handle_me() {
  if (!The_Memory_System()->contains(addr) && The_Memory_System()->object_table->probably_contains_not(addr)) {
    lprintf("%d about to do bad remote invalidate %d (%s) 0x%x (%s) 0x%x\n",
            getpid(), sender, Message_Statics::message_names[sender], addr, Message_Statics::message_names[(int)addr], nbytes);
    OS_Interface::die("bad remote invalidate");
  }
  OS_Interface::invalidate_mem(addr, nbytes);
}


void addObjectFromSnapshotMessage_class::handle_me() {
  Object* dst_obj = The_Memory_System()->add_object_from_snapshot_to_a_local_heap_allocating_chunk(dst_oop, src_obj_wo_preheader);
  addObjectFromSnapshotResponse_class(dst_obj).send_to(sender);
}

void addObjectFromSnapshotResponse_class::handle_me() {}

void addedScheduledProcessMessage_class::handle_me()  {
  ++The_Squeak_Interpreter()->added_process_count;
}




void broadcastInterpreterDatumMessage_class::handle_me() {
  void* p = (char*)The_Squeak_Interpreter() + datum_byte_offset;
  assert_always(p < (void*)(The_Squeak_Interpreter() + 1));
  switch (datum_size) {
    default: fatal();
    case sizeof(   char): *(   char*)p = (char)datum;  break;
    case sizeof(    int): *(    int*)p = ( int)datum;  break;
    case sizeof(u_int64): *(u_int64*)p =       datum;  break;
  }
}



class Report_Root_Closure: public Oop_Closure {
private:
  Oop_Closure* closure;
  int sender;

public:

  Report_Root_Closure(Oop_Closure* closure, int sender)
  : Oop_Closure(),
  closure(closure),
  sender(sender) {}

  void value(Oop* p, Object_p containing_obj_or_nil) {
    SEND_THEN_WAIT_FOR_MESSAGE(hereIsARootResponse_class(*p, p, containing_obj_or_nil, closure), sender, newValueForOopMessage);
  }
  
  virtual const char* class_name(char* buf) { 
    char buf1[BUFSIZ];
    sprintf(buf, "Report_Root_Closure for %s", closure->class_name(buf1));
    return buf;
  }
};


void doAllRootsHereMessage_class::handle_me() {
  // no GCs permitted when other side is reporting back roots
  Report_Root_Closure rc(closure, sender);
  {
    Safepoint_Ability sa(false);
    The_Squeak_Interpreter()->do_all_roots(&rc);
  }
  noMoreRootsResponse_class().send_to(sender);
}


void hereIsARootResponse_class::handle_me() {
  closure->value(&root, (Object_p)container_or_null);
  newValueForOopMessage_class(root, addr).send_to(sender);
}


void newValueForOopMessage_class::handle_me() {
  *addr = newValue;
}





void enforceCoherenceAfterEachCoreHasStoredIntoItsOwnHeapMessage_class::handle_me() {
  The_Memory_System()->heaps[Logical_Core::my_rank()][Memory_System::read_mostly]->enforce_coherence_in_whole_heap_after_store();
}

void enforceCoherenceBeforeEachCoreStoresIntoItsOwnHeapMessage_class::handle_me() {
  The_Memory_System()->invalidate_heaps_and_fence(false);
}

void enforceCoherenceBeforeSenderStoresIntoAllHeapsMessage_class::handle_me() {
  The_Memory_System()->invalidate_heaps_and_fence(true);
}



void flushFreeContextsMessage_class::handle_me() {
  The_Squeak_Interpreter()->roots.flush_freeContexts();
}

void flushInterpreterCachesMessage_class::handle_me() {
  The_Squeak_Interpreter()->flushInterpreterCaches();
}

void flushMethodCacheMessage_class::handle_me() {
  The_Squeak_Interpreter()->methodCache.flush_method_cache();
}

void flushSelectiveMessage_class::handle_me() {
  The_Squeak_Interpreter()->methodCache.flushSelective(selector);
}

void flushByMethodMessage_class::handle_me() {
  The_Squeak_Interpreter()->methodCache.flushByMethod(method);
}


void setExtraWordSelectorMessage_class::handle_me() {
# if Extra_Preheader_Word_Experiment
  The_Squeak_Interpreter()->roots.extra_preheader_word_selector = selector;
# else
  fatal("cannot set extra word, Extra_Preheader_Word_Experiment compiled as 0");
# endif
}


void setEmergencySemaphoreMessage_class::handle_me() {
  The_Squeak_Interpreter()->roots.emergency_semaphore = semaphore;
}



void loadFunctionFromPluginMessage_class::handle_me() {
  fn_t f = ioLoadFunctionFrom(fn_name, plugin_name);
  loadFunctionFromPluginResponse_class(f).send_to(sender);
}


void loadFunctionFromPluginResponse_class::handle_me() {}


void imageNamePutMessage_class::handle_me() {
  The_Memory_System()->imageNamePut_on_this_core(image_name, (int)len);
}


void noMessage_class::handle_me() {}
void noMoreRootsResponse_class::handle_me() {}

void postGCActionMessage_class::handle_me() {
  Safepoint_Ability sa(sender_is_able_to_safepoint &&  Safepoint_Ability::is_interpreter_able());
  The_Squeak_Interpreter()->postGCAction_here(fullGC);
}


void preGCActionMessage_class::handle_me() {
  The_Squeak_Interpreter()->preGCAction_here(fullGC);
}




void recycleContextIfPossibleMessage_class::handle_me() {
  The_Squeak_Interpreter()->recycleContextIfPossible_here(ctx);
}





void requestSafepointOnOtherCoresMessage_class::handle_me() {
  if (Safepoint_Tracker::verbose)
    lprintf("requestSafepointMessage_class::handle_me for %d, depth %d\n",  sender, The_Squeak_Interpreter()->safepoint_tracker->spin_depth());

  The_Squeak_Interpreter()->safepoint_master_control->request_other_cores_to_safepoint(sender, why);
}


void grantSafepointMessage_class::handle_me() {
  The_Squeak_Interpreter()->safepoint_tracker->every_other_core_is_safe(sequence_number);
}



void releaseOtherCoresFromSafepointMessage_class::handle_me() {
  The_Squeak_Interpreter()->safepoint_master_control->release_other_cores_from_safepoint(sender);
}


void requestCoreToSpinMessage_class::handle_me() {
  The_Squeak_Interpreter()->safepoint_tracker->another_core_needs_me_to_spin(for_whom, sequence_number, why);
}


void tellCoreToStopSpinningMessage_class::handle_me() {
  The_Squeak_Interpreter()->safepoint_tracker->another_core_no_longer_needs_me_to_spin(sequence_number);
}




void tellCoreIAmSpinningMessage_class::handle_me() {
  The_Squeak_Interpreter()->safepoint_master_control->a_core_is_now_safe(sender, sequence_number, was_spinning);
}



void runPrimitiveMessage_class::handle_me() {
  
  The_Squeak_Interpreter()->assert_stored_if_no_proc();
  static const bool verbose = false;

  if (verbose) {
    lprintf("handling runPrimitiveMessage from %d for 0x%x %d\n", sender, fn,
            The_Squeak_Interpreter()->increment_global_sequence_number());
  }

  if (The_Squeak_Interpreter()->safepoint_tracker->is_every_other_core_safe())
    fatal("the other cores are safe; should not be asking me for something");


  Interpreter_Subset_For_Control_Transfer saved_interp_info;

  saved_interp_info.set_from_interpreter();
  int saved_arg_count = The_Squeak_Interpreter()->get_argumentCount();

  int saved_root_count = 0;
# define pushAndCount(type, my_var, in_var) ++saved_root_count, The_Squeak_Interpreter()->pushRemappableOop(The_Squeak_Interpreter()->in_var);
  FOR_ALL_OOPS_IN_SUBSET(pushAndCount)
# undef pushAndCount


  subset.fill_in_interpreter();



  Oop proc_before_prim = The_Squeak_Interpreter()->get_running_process();
  The_Squeak_Interpreter()->set_argumentCount(argCount);
  Message_Statics::remote_prim_fn = fn;
  The_Squeak_Interpreter()->run_primitive_on_main_from_elsewhere(fn);

  if (verbose) {
    lprintf("sending runPrimitiveResponse 0x%x %d\n", fn,
            The_Squeak_Interpreter()->increment_global_sequence_number());
  }
  runPrimitiveResponse_class().send_to(sender);

  if (verbose) {
    lprintf("sent runPrimitiveResponse 0x%x %d\n", fn,
            The_Squeak_Interpreter()->increment_global_sequence_number());
  }

  The_Squeak_Interpreter()->popRemappableOops(saved_root_count);
  saved_interp_info.fill_in_interpreter();
  The_Squeak_Interpreter()->set_argumentCount(saved_arg_count);

  The_Squeak_Interpreter()->assert_stored_if_no_proc();

  Message_Statics::remote_prim_fn = 0;

}


void runPrimitiveResponse_class::handle_me() {
  static const bool verbose = false;
  subset.fill_in_interpreter();

  if (verbose) {
    lprintf("handled runPrimitiveResponse %d\n",
            The_Squeak_Interpreter()->increment_global_sequence_number());
  }
}


void sampleOneCoreMessage_class::handle_me() { 
  Oop sample = sample_one_core(what_to_sample);
  The_Squeak_Interpreter()->pushRemappableOop(sample); // could GC while msg in transit
  sampleOneCoreResponse_class(sample).send_to(sender);   
  The_Squeak_Interpreter()->popRemappableOop();
}

void sampleOneCoreResponse_class::handle_me() {
  The_Squeak_Interpreter()->pushRemappableOop(result); // protect from a GC (may already be spinning)
}


void scanCompactOrMakeFreeObjectsMessage_class::handle_me() {
  The_Memory_System()->scan_compact_or_make_free_objects_here(compacting, gc_or_null);
}


void startInterpretingMessage_class::handle_me() {}

void transferControlMessage_class::handle_me() {
  assert(!Logical_Core::my_rank() != sender);
  subset.fill_in_interpreter();
}


void updateEnoughInterpreterToTransferControlMessage_class::handle_me() {
  fatal("only subclasses should actually be used");
}


void distributeInitialInterpreterMessage_class::handle_me() {
  The_Squeak_Interpreter()->receive_initial_interpreter_from_main(interp);
}


void verifyInterpreterAndHeapMessage_class::handle_me() {
  The_Squeak_Interpreter()->verify();
  The_Memory_System()->heaps[Logical_Core::my_rank()][Memory_System::read_mostly]->verify();
  The_Memory_System()->heaps[Logical_Core::my_rank()][Memory_System:: read_write]->verify();
}




void zapUnusedPortionOfHeapMessage_class::handle_me() {
  The_Memory_System()->heaps[Logical_Core::my_rank()][Memory_System:: read_write]->zap_unused_portion();
  The_Memory_System()->heaps[Logical_Core::my_rank()][Memory_System::read_mostly]->zap_unused_portion();
}



void screenInfoMessage_class::handle_me() {
  screenInfoResponse_class(ioScreenSize(), The_Memory_System()->snapshot_window_size.fullScreenFlag()).send_to(sender);
}

void screenInfoResponse_class::handle_me() {}

void getNextEventMessage_class::handle_me() {
  assert(Logical_Core::running_on_main());
  getNextEventResponse_class m;
  m.got_one = The_Squeak_Interpreter()->getNextEvent_any_platform(m.evtBuf);
  m.send_to(sender);
}

void getNextEventResponse_class::handle_me() {
  if (false && got_one && evtBuf[0] == 2)
    lprintf("getNextEventResponse_class::handle_me key: %d, state %d\n", evtBuf[2], evtBuf[3]);
}

void selfDestructMessage_class::handle_me() {
  lprintf("received self-destruct from %d: %s\n", sender, why);
  fatal("self-destruct");
}

void ackMessage_class::handle_me() {}





void flushSelectiveMessage_class::do_all_roots(Oop_Closure* oc) {
  oc->value(&selector, (Object_p)NULL);
}
void sampleOneCoreResponse_class::do_all_roots(Oop_Closure* oc) {
  oc->value(&result, (Object_p)NULL);
}
void newValueForOopMessage_class::do_all_roots(Oop_Closure* oc) {
  oc->value(&newValue, (Object_p)NULL);
}
void addObjectFromSnapshotMessage_class::do_all_roots(Oop_Closure* oc) {
  oc->value(&dst_oop, (Object_p)NULL);
}
void recycleContextIfPossibleMessage_class::do_all_roots(Oop_Closure* oc) {
  oc->value(&ctx, (Object_p)NULL);
}
void flushByMethodMessage_class::do_all_roots(Oop_Closure* oc) {
  oc->value(&method, (Object_p)NULL);
}
void addObjectFromSnapshotResponse_class::do_all_roots(Oop_Closure* /* oc */) {
  fatal("unimp, but should not be called");
}
void hereIsARootResponse_class::do_all_roots(Oop_Closure* /* oc */) {
  fatal("unimp, but should not be called");
}
void updateEnoughInterpreterToTransferControlMessage_class::do_all_roots(Oop_Closure* oc) {
  subset.do_all_roots(oc);
}

