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
# include "sys/time.h"
# include <time.h>
# include <math.h>


static const char *moduleName =
#ifdef SQUEAK_BUILTIN_PLUGIN
"RVMPlugin v1.5 14 April 2008 (i)"
#else
"RVMPlugin v1.5 14 April 2008 (e)"
#endif
;

static u_int64 cycle_count_at_last_sample[Memory_Semantics::max_num_threads_on_threads_or_1_on_processes] = { 0 };  // threadsafe


/**
 * Was used to find a bug with sampling, primitives, and safepoint interaction.
 * Might still be usefull for other debugging purposes.  2011-05-23 dmu+sm
 */
void* primitiveDebugSampleRVM() {
  static int n = 0;
  printf("<%d", ++n);
  The_Squeak_Interpreter()->assert_external();
  Safepoint_for_moving_objects sp("primitiveDebugSampleRVM"); // sends mesgs to other cores to allocate arrays, might cause GC
  
  switch (The_Squeak_Interpreter()->get_argumentCount()) {
    case 0:
     break;
 
    default:
      The_Squeak_Interpreter()->primitiveFail();
      break;
  }

  printf(">");
  return NULL;
}



void* primitiveSampleRVM() {
  The_Squeak_Interpreter()->assert_external();
  
  // Commented out the next line because otherwise there is potential for a deadlock
  // if a core whom I ask for stats needs to do a GC to allocate the stats object. 
  // Seems to work, despite my earlier comment below. -- dmu 8/10
  
  // Safepoint_for_moving_objects sp("primitiveSampleRVM"); // sends mesgs to other cores to allocate arrays, might cause GC, doesn't work right without this -- dmu 5/10
  
  int what_to_sample;
   switch (The_Squeak_Interpreter()->get_argumentCount()) {
    case 0:
      what_to_sample = ~0;
      break;

    case 1:
      what_to_sample = The_Squeak_Interpreter()->stackIntegerValue(0);
      break;

    default:
      The_Squeak_Interpreter()->primitiveFail();
      The_Squeak_Interpreter()->assert_external();
      return 0;
  }
  The_Squeak_Interpreter()->assert_external();
  if (The_Squeak_Interpreter()->failed()) { return 0; }

  The_Squeak_Interpreter()->pop(The_Squeak_Interpreter()->get_argumentCount() + 1);

  The_Squeak_Interpreter()->assert_external();
  if ((what_to_sample & (1 << SampleValues::allCoreStats)) == 0) {
    The_Squeak_Interpreter()->push(sample_one_core(what_to_sample));
    return 0;
  }
  The_Squeak_Interpreter()->assert_external();
  
  int s = The_Squeak_Interpreter()->makeArrayStart();
  if (what_to_sample & (1 << SampleValues::runMask)) {
    Oop runMask = Object::positive64BitIntegerFor(The_Squeak_Interpreter()->run_mask());
    PUSH_WITH_STRING_FOR_MAKE_ARRAY(runMask);
  }
  The_Squeak_Interpreter()->assert_external();
  
  if (what_to_sample & (1 << SampleValues::messageNames)) {
    Oop messageNames = Message_Stats::get_message_names();
    PUSH_WITH_STRING_FOR_MAKE_ARRAY(messageNames);
  }
  The_Squeak_Interpreter()->assert_external();
  
  if (what_to_sample & (1 << SampleValues::cpuCoreStats)) {
    Oop cpuCoreStats = CPU_Coordinate::get_stats();
    PUSH_WITH_STRING_FOR_MAKE_ARRAY(cpuCoreStats);
  }
  The_Squeak_Interpreter()->assert_external();
  
  if (what_to_sample & (1 << SampleValues::allCoreStats)) {
    Oop allCoreStats = The_Interactions.sample_each_core(what_to_sample);
    PUSH_WITH_STRING_FOR_MAKE_ARRAY(allCoreStats);
  }
  The_Squeak_Interpreter()->assert_external();

  if (what_to_sample & (1 << SampleValues::fence)) {
    Oop fence = The_Squeak_Interpreter()->fence() ? The_Squeak_Interpreter()->roots.trueObj :  The_Squeak_Interpreter()->roots.falseObj;
    PUSH_WITH_STRING_FOR_MAKE_ARRAY(fence);
  }
  The_Squeak_Interpreter()->assert_external();

  The_Squeak_Interpreter()->push(The_Squeak_Interpreter()->makeArray(s));
  
  return NULL;
}


Oop sample_one_core(int what_to_sample) {
  const int rank_on_threads_or_zero_on_processes = Memory_Semantics::rank_on_threads_or_zero_on_processes();

  static int ms_buf[Memory_Semantics::max_num_threads_on_threads_or_1_on_processes] = { 0 };                                   // threadsafe
  int* const ms = &(ms_buf[rank_on_threads_or_zero_on_processes]);

  int millisecs = The_Squeak_Interpreter()->ioWhicheverMSecs() - (*ms);  (*ms) = (*ms) + millisecs;

  const u_int64 current_cycles = OS_Interface::get_cycle_count();
  const u_int64 cycles = current_cycles - cycle_count_at_last_sample[rank_on_threads_or_zero_on_processes];
  cycle_count_at_last_sample[rank_on_threads_or_zero_on_processes] = current_cycles;

  int s = The_Squeak_Interpreter()->makeArrayStart();
  if (what_to_sample & (1 << SampleValues::millisecs))
    PUSH_POSITIVE_32_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(millisecs);

  if (what_to_sample & (1 << SampleValues::cycles))
    PUSH_POSITIVE_64_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(cycles);

  if (what_to_sample & (1 << SampleValues::messageStats)) {
    Oop messageStats = Message_Stats::get_stats(what_to_sample);
    PUSH_WITH_STRING_FOR_MAKE_ARRAY( messageStats );
  }
  if (what_to_sample & (1 << SampleValues::memorySystemStats)) {
    Oop memorySystemStats = The_Memory_System()->get_stats(what_to_sample);
    PUSH_WITH_STRING_FOR_MAKE_ARRAY(memorySystemStats);
  }
  if (what_to_sample & (1 << SampleValues::interpreterStats)) {
    Oop interpreterStats = The_Squeak_Interpreter()->get_stats(what_to_sample);
    PUSH_WITH_STRING_FOR_MAKE_ARRAY(interpreterStats);
  }
  if (what_to_sample & (1 << SampleValues::objectTableStats)) {
    Oop objectTableStats = The_Memory_System()->object_table->get_stats(Logical_Core::my_rank());
    PUSH_WITH_STRING_FOR_MAKE_ARRAY(objectTableStats);
  }
  if (what_to_sample & (1 << SampleValues::interactionStats)) {
    Oop interactionsStats = The_Interactions.get_stats();
    PUSH_WITH_STRING_FOR_MAKE_ARRAY(interactionsStats);
  }

  return The_Squeak_Interpreter()->makeArray(s);
}


void* primitiveBreakpoint() {
  if (The_Squeak_Interpreter()->get_argumentCount()) { The_Squeak_Interpreter()->primitiveFail(); }
  breakpoint();
  return NULL;
}

void* primitiveRunMask() {
  if (The_Squeak_Interpreter()->get_argumentCount() == 0) {
    The_Squeak_Interpreter()->pop(1);
    The_Squeak_Interpreter()->push(Object::positive64BitIntegerFor(The_Squeak_Interpreter()->run_mask()));
    The_Squeak_Interpreter()->success(true);
    return 0;
  }
  if (The_Squeak_Interpreter()->get_argumentCount() == 1) {
    u_int64 old = The_Squeak_Interpreter()->run_mask();
    u_int64 n = The_Squeak_Interpreter()->positive64BitValueOf(The_Squeak_Interpreter()->stackTop());
    if (The_Squeak_Interpreter()->failed())  return 0;
    if (n == 0)  n = ~n; // zero means all
    u_int64 possible_cores = (1LL << u_int64(Logical_Core::group_size)) - 1;
    if ( (n & possible_cores)  ==  0 ) {
      The_Squeak_Interpreter()->primitiveFail();
      return 0;
    }
    The_Squeak_Interpreter()->pop(2);
    The_Squeak_Interpreter()->push(Object::positive64BitIntegerFor(old));
    The_Squeak_Interpreter()->success(true);
    The_Squeak_Interpreter()->set_run_mask_and_request_yield(n);
    return 0;
  }
  The_Squeak_Interpreter()->success(false);
  return 0;
}


void* primitiveSetCoordinatesFor() {
  const bool print = false;
  if (print)  lprintf("starting\n");
  // Args are: object, rank (int), [mutability (int)]
  Oop oop;
  int rank;
  int mutability;
  const int c = Memory_System::read_write;  const int i = Memory_System::read_mostly; // compiler bug

  switch (The_Squeak_Interpreter()->get_argumentCount()) {
    case 2:
      oop = The_Squeak_Interpreter()->stackObjectValue(1);
      if (!The_Squeak_Interpreter()->successFlag) { return 0; }
      rank = The_Squeak_Interpreter()->stackIntegerValue(0);
      mutability = oop.mutability();
      break;

    case 3:
      oop = The_Squeak_Interpreter()->stackObjectValue(2);
      rank = The_Squeak_Interpreter()->stackIntegerValue(1);
      mutability = The_Squeak_Interpreter()->booleanValueOf(The_Squeak_Interpreter()->stackValue(0)) ? c : i;
      if (!The_Squeak_Interpreter()->successFlag) { return 0; }
      break;

    default: The_Squeak_Interpreter()->primitiveFail();  return 0;
  }
  if (print)  lprintf("params %d %d\n", rank, mutability);
  Object_p obj = oop.as_object();
  int32 total_bytes = obj->extra_header_bytes() + obj->sizeBits();

  if ( rank < 0
  ||  rank >= Logical_Core::group_size
  ||  (mutability == Memory_System::read_mostly  &&  !obj->is_suitable_for_replication())
  ||  !The_Memory_System()->heaps[rank][mutability]->sufficientSpaceToAllocate(2500 + total_bytes)) {
    The_Squeak_Interpreter()->primitiveFail();
  }
  else {
    The_Squeak_Interpreter()->popThenPush(The_Squeak_Interpreter()->get_argumentCount() + 1, oop);
    obj->move_to_heap(rank, mutability, true);
    if (print)  lprintf("success %d %d\n", rank, mutability);
  }
  return 0;
}


void* primitiveGetCore() {
  // Return rank of receiver's core.
  // Fail if receiver is smallInt.

  if (The_Squeak_Interpreter()->get_argumentCount() != 1) { The_Squeak_Interpreter()->primitiveFail(); return 0; }
  Oop rcvr = The_Squeak_Interpreter()->stackTop();
  if (!rcvr.is_mem()) { The_Squeak_Interpreter()->primitiveFail(); return 0; }

  Object_p ro = rcvr.as_object();
  The_Squeak_Interpreter()->popThenPushInteger(2, The_Memory_System()->rank_for_address(ro));
  return 0;
}


void* primitiveGetCoreIAmRunningOn() {
  if (The_Squeak_Interpreter()->get_argumentCount() != 0) { The_Squeak_Interpreter()->primitiveFail(); return 0; }
  The_Squeak_Interpreter()->popThenPushInteger(1, Logical_Core::my_rank());
  return 0;
}


void* primitiveGetMutability() {
  if (The_Squeak_Interpreter()->get_argumentCount() != 1) { The_Squeak_Interpreter()->primitiveFail(); return 0; }
  Oop rcvr = The_Squeak_Interpreter()->stackTop();
  if (!rcvr.is_mem()) { The_Squeak_Interpreter()->primitiveFail(); return 0; }

  Object_p ro = rcvr.as_object();
  The_Squeak_Interpreter()->pop(2);
  The_Squeak_Interpreter()->pushBool(ro->is_read_write());
  return 0;
}

extern int headless;

void* primitiveRunsHeadless() {
  if (The_Squeak_Interpreter()->get_argumentCount() != 0) { The_Squeak_Interpreter()->primitiveFail(); return 0; }
  The_Squeak_Interpreter()->pop(1);
  The_Squeak_Interpreter()->pushBool(headless);
  return 0;
}


// args for the primitive are first core, last core, move_read_write_to_read_mostly, move_read_mostly_to_read_write
void* shuffle_or_spread(bool spread)  {
  Oop first, last;
  int f = 0, L = Logical_Core::group_size - 1;
  bool move_read_write_to_read_mostly = false,  move_read_mostly_to_read_write = false;
  switch (The_Squeak_Interpreter()->get_argumentCount()) {
    default: break;
    case 4:
      move_read_write_to_read_mostly = The_Squeak_Interpreter()->booleanValueOf(The_Squeak_Interpreter()->stackValue(1));
      move_read_mostly_to_read_write = The_Squeak_Interpreter()->booleanValueOf(The_Squeak_Interpreter()->stackValue(0));
      if (!The_Squeak_Interpreter()->successFlag) return 0;
      // FALL THROUGH
    case 2:
      first = The_Squeak_Interpreter()->stackValue(The_Squeak_Interpreter()->get_argumentCount() - 1);
      last  = The_Squeak_Interpreter()->stackValue(The_Squeak_Interpreter()->get_argumentCount() - 2);
      if (!first.is_int() ||  !last.is_int())  break;
      f = first.integerValue();
      L = last.integerValue();
      if (!(0 <= f  &&  f <= L  &&  L < Logical_Core::group_size))
        break;
      // FALL THROUGH
    case 0:
      if (!The_Memory_System()->shuffle_or_spread(f, L, move_read_write_to_read_mostly, move_read_mostly_to_read_write, spread))
        break;
      The_Squeak_Interpreter()->pop(The_Squeak_Interpreter()->get_argumentCount());
      return 0;
  }
  The_Squeak_Interpreter()->primitiveFail();
  return 0;
}


void* primitiveShuffle() { return shuffle_or_spread(false); }
void* primitiveSpread()  { return shuffle_or_spread(true); }


void* primitiveMoveAllToReadMostlyHeaps() {
  switch (The_Squeak_Interpreter()->get_argumentCount()) {
    default: break;
    case 0:
      if (!The_Memory_System()->moveAllToRead_MostlyHeaps())
        break;
      return 0;
  }
  The_Squeak_Interpreter()->primitiveFail();
  return 0;
}


void* primitiveTraceCores() {
  switch (The_Squeak_Interpreter()->get_argumentCount()) {
    case 1: { // start tracing, size arg
      int n = The_Squeak_Interpreter()->stackIntegerValue(0);
      if (!The_Squeak_Interpreter()->successFlag  ||  n < 0) break;
      Core_Tracer* t = The_Squeak_Interpreter()->core_tracer();
      if (t != NULL) {
        The_Squeak_Interpreter()->set_core_tracer(NULL);
        delete t;
      }
      if (n > 0)
        The_Squeak_Interpreter()->set_core_tracer(new Core_Tracer(n));
      The_Squeak_Interpreter()->pop(1);
      return 0;
    }

    case 0:
      if (The_Squeak_Interpreter()->core_tracer() == NULL)
        break;
      The_Squeak_Interpreter()->popThenPush(1, The_Squeak_Interpreter()->core_tracer()->get());
      return 0;

    default:
      break;
  }
  The_Squeak_Interpreter()->primitiveFail();
  return 0;
}

void* primitivePrintReadWriteReadMostlyBytesUsed() {
  FOR_ALL_RANKS(r)
    lprintf("%d: %d @ %d\n",
            r,
            The_Memory_System()->heaps[r][Memory_System::read_write ]->bytesUsed(),
            The_Memory_System()->heaps[r][Memory_System::read_mostly]->bytesUsed());
  return 0;
}


// given rcvr, rank, isRead_Write, return array of all objs in that heap

void* primitiveAllObjectsInHeap() {
  switch (The_Squeak_Interpreter()->get_argumentCount()) {
    default: break;
    case 2: {
      bool isRead_Write = The_Squeak_Interpreter()->booleanValueOf(The_Squeak_Interpreter()->stackTop());
      if (!The_Squeak_Interpreter()->successFlag)
        break;
      int rank = The_Squeak_Interpreter()->stackIntegerValue(1);
      if (The_Squeak_Interpreter()->successFlag &&  0 <= rank  &&  rank < Logical_Core::group_size)
        ;
      else
        break;
      static const int rw = Memory_System::read_write;
      static const int rm = Memory_System::read_mostly;
      int mutability = isRead_Write ? rw : rm;
      Multicore_Object_Heap* h = The_Memory_System()->heaps[rank][mutability];
      int n = 0;
      FOR_EACH_OBJECT_IN_HEAP(h, p)
        if (!p->isFreeObject())
          ++n;
      Object_p r = The_Squeak_Interpreter()->splObj(Special_Indices::ClassArray).as_object()->instantiateClass(n);
      int i = 0;
      FOR_EACH_OBJECT_IN_HEAP(h, p) {
        if (p->isFreeObject())
          continue;
        if (i >= n)
          break;
        r->storePointer(i, p->as_oop());
        ++i;
      }
      The_Squeak_Interpreter()->popThenPush(3, r->as_oop());
      return 0;
    }
  }
  The_Squeak_Interpreter()->primitiveFail();
  return 0;
}


void* primitiveTraceMutatedReplicatedObjects() {
  switch (The_Squeak_Interpreter()->get_argumentCount()) {
    case 1: { // start tracing, size arg
      int n = The_Squeak_Interpreter()->stackIntegerValue(0);
      if (!The_Squeak_Interpreter()->successFlag  ||  n < 0) break;
      Oop_Tracer* t = The_Squeak_Interpreter()->mutated_read_mostly_object_tracer();
      if (t != NULL) {
        The_Squeak_Interpreter()->set_mutated_read_mostly_object_tracer(NULL);
        delete t;
      }
      if (n > 0)
        The_Squeak_Interpreter()->set_mutated_read_mostly_object_tracer(new Oop_Tracer(n));
      The_Squeak_Interpreter()->pop(1);
      return 0;
    }

    case 0:
      if (The_Squeak_Interpreter()->mutated_read_mostly_object_tracer() == NULL)
        break;
      The_Squeak_Interpreter()->popThenPush(1, The_Squeak_Interpreter()->mutated_read_mostly_object_tracer()->get());
      return 0;

      default:
      break;
  }
  The_Squeak_Interpreter()->primitiveFail();
  return 0;
}



static const char* getModuleName() { return moduleName; }

/*	Note: This is coded so that is can be run from Squeak. */

static int setInterpreter(struct VirtualMachine* /* anInterpreter */) {
	return 1;
}

static int primitivePrintStack() { 
  The_Squeak_Interpreter()->print_stack_trace(dittoing_stdout_printer);
  return 0;
}

static int primitivePrint() {
  if (The_Squeak_Interpreter()->get_argumentCount() != 1) {
    The_Squeak_Interpreter()->primitiveFail();
    return 0;
  }
  Oop x = The_Squeak_Interpreter()->stackTop();
  if (!x.isBytes()) {
    The_Squeak_Interpreter()->primitiveFail();
    return 0;
  }
  stdout_printer->lprintf("primitivePrint: ");
  x.as_object()->print_bytes(stdout_printer);
  stdout_printer->nl();
  The_Squeak_Interpreter()->pop(1);
  return 0;
}


// A numbers print for debugging
static int primitivePrintObjectForVMDebugging() {
  if (The_Squeak_Interpreter()->get_argumentCount() != 1) {
    The_Squeak_Interpreter()->primitiveFail();
    return 0;
  }
  Oop x = The_Squeak_Interpreter()->stackTop();
  stdout_printer->lprintf("primitivePrintObjectForVMDebugging: Oop 0x%x, Object* 0x%x, ", x.bits(),
                            x.is_mem() ? x.as_untracked_object_ptr() : NULL
                          );

  
  x.print(stdout_printer);
  stdout_printer->nl();
  The_Squeak_Interpreter()->pop(1);
  return 0;
}



static int primitivePrintStats() {
  lprintf( "Semaphore_Mutex: ");
  The_Squeak_Interpreter()->get_semaphore_mutex()->print_stats();

  lprintf( "Scheduler_Mutex: ");
  The_Squeak_Interpreter()->get_scheduler_mutex()->print_stats();

  lprintf( "Safepoint_Mutex: ");
  The_Squeak_Interpreter()->get_safepoint_mutex()->print_stats();

/*  lprintf( "interpret_cycles = %lld,  multicore_interrupt_cycles = %lld, mi_cyc_1a = %lld, mi_cyc_1a1 = %lld, mi_cyc_1a2 = %lld, mi_cyc_1b = %lld, mi_cyc_1 = %lld\n",
          The_Squeak_Interpreter()->interpret_cycles,  The_Squeak_Interpreter()->multicore_interrupt_cycles,
          The_Squeak_Interpreter()->mi_cyc_1a, The_Squeak_Interpreter()->mi_cyc_1a1, The_Squeak_Interpreter()->mi_cyc_1a2, 
          The_Squeak_Interpreter()->mi_cyc_1b, The_Squeak_Interpreter()->mi_cyc_1);*/

/*  lprintf( "multicore_interrupt_check_count = %d, yield_request_count = %d, data_available_count = %d\n",
            The_Squeak_Interpreter()->multicore_interrupt_check_count,
          The_Squeak_Interpreter()->yield_request_count,
          The_Squeak_Interpreter()->data_available_count);*/

  int rank_on_threads_or_zero_on_processes = Memory_Semantics::rank_on_threads_or_zero_on_processes();

  lprintf("buf_msg_check_count = %d, buf_msg_check_cyc = %lld\n", 
          Message_Stats::stats[rank_on_threads_or_zero_on_processes].buf_msg_check_count, 
          Message_Stats::stats[rank_on_threads_or_zero_on_processes].buf_msg_check_cyc);

  bool did_one = false;
  for (int i = 0;  i < Message_Statics::end_of_messages;  ++i) {
    if (Message_Stats::stats[rank_on_threads_or_zero_on_processes].receive_tallies[i]) {
      lprintf("\n%s: %d %lld", Message_Statics::message_names[i], 
              Message_Stats::stats[rank_on_threads_or_zero_on_processes].receive_tallies[i],
              Message_Stats::stats[rank_on_threads_or_zero_on_processes].receive_cycles[i]);
      did_one = true;
    }
  }
  if (!did_one) lprintf("no msgs received\n");
  else  lprintf("\n");

  lprintf("remote_prim_count = %d,  remote_prim_cycles = %lld\n",
          The_Interactions.remote_prim_count,  The_Interactions.remote_prim_cycles);

  The_Squeak_Interpreter()->pop(The_Squeak_Interpreter()->get_argumentCount());
  
  Performance_Counters::print();
  
  return 0;
}

static int primitiveResetPerfCounters() {
  Performance_Counters::reset();
  return 0;
}
  
static int primitivePrintExecutionTrace() { The_Squeak_Interpreter()->print_execution_trace(); return 0; }


static int primitiveThisProcess() {
  The_Squeak_Interpreter()->pop(The_Squeak_Interpreter()->get_argumentCount() + 1);
  The_Squeak_Interpreter()->push(The_Squeak_Interpreter()->get_running_process());
  The_Squeak_Interpreter()->set_primitiveThisProcess_was_called(true);
  return 0;
}

static int primitiveCoreCount() {
  if (The_Squeak_Interpreter()->get_argumentCount()) { The_Squeak_Interpreter()->primitiveFail(); return 0; }
  The_Squeak_Interpreter()->popThenPush(1, Oop::from_int(Logical_Core::group_size));
  return 0;
}

static int primitiveGetExtraPreheaderWord() {
  if (The_Squeak_Interpreter()->get_argumentCount() != 1) {
    The_Squeak_Interpreter()->primitiveFail();
    return 0;
  }
  Oop x = The_Squeak_Interpreter()->stackObjectValue(0);

  if (The_Squeak_Interpreter()->failed())
    return 0;
  
  oop_int_t* p = x.as_object()->extra_preheader_word();

  if (p == NULL) {
    The_Squeak_Interpreter()->primitiveFail();
    return 0;
  }
  The_Squeak_Interpreter()->popThenPush(2, Oop::from_bits(*p));
  return 0;
}


// for compatability's sake, if one arg, set the extra word in the arg to the value of the receiver
// Newer form: if two args, set the extra word in arg 1 to the value of arg2 -- dmu 6/10
static int primitiveSetExtraPreheaderWord() {
  Oop w, x;
  switch (The_Squeak_Interpreter()->get_argumentCount()) {
    default:
      The_Squeak_Interpreter()->primitiveFail();
      return 0;
  
    case 1: 
      w = The_Squeak_Interpreter()->stackValue(1); // receiver is word to set
      x = The_Squeak_Interpreter()->stackObjectValue(0); // arg gets word set
      break;
  
    case 2: 
      w = The_Squeak_Interpreter()->stackValue(0); // arg 2 is word to set
      x = The_Squeak_Interpreter()->stackObjectValue(1); // arg 1 gets word set
      break;
  }
  if (The_Squeak_Interpreter()->failed()) return 0;
  
  oop_int_t* p = x.as_object()->extra_preheader_word();  if (p == NULL) {The_Squeak_Interpreter()->primitiveFail(); return 0; }
  
  Oop old = Oop::from_bits(*p);
  x.as_object()->set_extra_preheader_word(w.bits());
  The_Squeak_Interpreter()->popThenPush(The_Squeak_Interpreter()->get_argumentCount() + 1,  old);
  return 0;
}


static int primitiveSetExtraWordSelector() {
  if (The_Squeak_Interpreter()->get_argumentCount() != 1) {
    The_Squeak_Interpreter()->primitiveFail();
    return 0;
  }
  Oop nnew = The_Squeak_Interpreter()->stackValue(0);
  Oop selectorClass = The_Squeak_Interpreter()->splObj(Special_Indices::SelectorDoesNotUnderstand).fetchClass();
  Oop nnewClass = nnew.fetchClass();
  if (selectorClass != nnewClass  &&  nnew != The_Squeak_Interpreter()->roots.nilObj)  {
    The_Squeak_Interpreter()->primitiveFail();
    return 0;
  }
# if Extra_Preheader_Word_Experiment
    The_Squeak_Interpreter()->pushRemappableOop(The_Squeak_Interpreter()->roots.extra_preheader_word_selector);
    The_Squeak_Interpreter()->pushRemappableOop(nnew);
  
    setExtraWordSelectorMessage_class m(nnew);
    m.send_to_all_cores();
    The_Squeak_Interpreter()->popRemappableOop();

    The_Squeak_Interpreter()->popThenPush(2, The_Squeak_Interpreter()->popRemappableOop());
# else
    The_Squeak_Interpreter()->primitiveFail();
# endif
  return 0;
}


static int primitiveEmergencySemaphore() {
  int ac = The_Squeak_Interpreter()->get_argumentCount();
  if (ac == 0) {
    The_Squeak_Interpreter()->popThenPush(1, The_Squeak_Interpreter()->roots.emergency_semaphore);
    return 0;
  }
  if (ac != 1) {
    The_Squeak_Interpreter()->primitiveFail();
    return 0;
  }
  Oop nnew = The_Squeak_Interpreter()->stackValue(0);
  if (nnew.fetchClass() != The_Squeak_Interpreter()->splObj(Special_Indices::ClassSemaphore)) {
    The_Squeak_Interpreter()->primitiveFail();
    return 0;
  }
  The_Squeak_Interpreter()->pushRemappableOop( The_Squeak_Interpreter()->roots.emergency_semaphore );
  The_Squeak_Interpreter()->pushRemappableOop( nnew );
  setEmergencySemaphoreMessage_class m(nnew);
  m.send_to_all_cores();
  The_Squeak_Interpreter()->popRemappableOop();
  
  The_Squeak_Interpreter()->popThenPush(ac + 1, The_Squeak_Interpreter()->popRemappableOop());

  return 0;
}


// WARNING: currently only works if always_track_running_processes_is_set
static int primitiveRunningProcessByCore() {
  if (!Track_Processes) {
    The_Squeak_Interpreter()->primitiveFail();
    return 0;
  }
  int s = The_Squeak_Interpreter()->makeArrayStart();
  FOR_ALL_RANKS(r)
    PUSH_FOR_MAKE_ARRAY(The_Squeak_Interpreter()->running_process_by_core[r]);
  The_Squeak_Interpreter()->popThenPush(1, The_Squeak_Interpreter()->makeArray(s));
  return 0;
}

static int primitiveWriteSnapshot() {
  // for debugging
  if (The_Squeak_Interpreter()->get_argumentCount() == 0)
    The_Memory_System()->imageNamePut_on_this_core("x.image", 7);
  else {
    Oop s = The_Squeak_Interpreter()->stackTop();
    Squeak_Image_Reader::imageNamePut_on_all_cores(s.as_object()->as_char_p() + Object::BaseHeaderSize, s.as_object()->stSize());
  }
  The_Squeak_Interpreter()->primitiveSnapshot();
  The_Squeak_Interpreter()->pop(The_Squeak_Interpreter()->get_argumentCount());
  return 0;
}


static int primitiveMicrosecondClock() {
  // return a microsecond clock
  struct timeval now;
  gettimeofday(&now, 0);
  int r = now.tv_usec;
  The_Squeak_Interpreter()->popThenPush(1, Oop::from_int(r));
  return 0;
}

# if On_OSX
# include "/Developer/Headers/FlatCarbon/MacTypes.h" // Ugh! Why won't xCode supply UnsignedWide??? -- dmu
# endif


static int primitiveCycleCounter() {
  if (The_Squeak_Interpreter()->get_argumentCount() != 0) {
    The_Squeak_Interpreter()->primitiveFail();
    return 0;
  }
  uint64_t cycles;

# if On_iOS
  The_Squeak_Interpreter()->primitiveFail();
  return 0;
  
  // burrow down below OS_Interface level to avoid effect of Count_Cycles flag
# elif On_Tilera
  cycles = ::get_cycle_count();
  
# elif On_Apple
  struct UnsignedWide microTickCount;
  Microseconds(&microTickCount);
  cycles = (u_int64(microTickCount.hi) << 32LL) |  u_int64(microTickCount.lo);
  
# else
  // this value is specifc to a core, it does not reflect a 'global' time.
  asm volatile("rdtsc" : "=A" (cycles));
# endif
  
  The_Squeak_Interpreter()->popThenPush(1, Object::positive64BitIntegerFor(cycles)); 
  return 0;
}

  

void* primitiveUseCPUTime() {
  bool use_cpu_ms;
  switch (The_Squeak_Interpreter()->get_argumentCount()) {
     case 1:
      use_cpu_ms = The_Squeak_Interpreter()->booleanValueOf(The_Squeak_Interpreter()->stackValue(0));
      if (!The_Squeak_Interpreter()->successFlag) { return 0; }
      break;
      
    default: The_Squeak_Interpreter()->primitiveFail();  return 0;
  }
  
  // There's a bit of a race in the code below; it's clunky, too. -- dmu
  bool fix_wakeup_time = false;
  int delta = 0;
  if (use_cpu_ms != The_Squeak_Interpreter()->use_cpu_ms()  &&  The_Squeak_Interpreter()->nextWakeupTick() != 0) {
    delta = The_Squeak_Interpreter()->nextWakeupTick() - The_Squeak_Interpreter()->ioWhicheverMSecs();
    fix_wakeup_time = true;
  }
  The_Squeak_Interpreter()->forceInterruptCheck();
  The_Squeak_Interpreter()->set_use_cpu_ms(use_cpu_ms);
  if (fix_wakeup_time)
    The_Squeak_Interpreter()->set_nextWakeupTick( The_Squeak_Interpreter()->ioWhicheverMSecs() + delta);
  The_Squeak_Interpreter()->pop(The_Squeak_Interpreter()->get_argumentCount());
  return 0;
}



void* RVMPlugin_exports[][3] = {
  {(void*) "RVMPlugin", (void*)"primitiveDebugSampleRVM", (void*)primitiveDebugSampleRVM},
  {(void*) "RVMPlugin", (void*)"primitiveSampleRVM", (void*)primitiveSampleRVM},
  {(void*) "RVMPlugin", (void*)"primitiveRunMask", (void*)primitiveRunMask},
  {(void*) "RVMPlugin", (void*)"primitiveBreakpoint", (void*)primitiveBreakpoint},
  {(void*) "RVMPlugin", (void*)"primitiveSetCoordinatesFor", (void*)primitiveSetCoordinatesFor},
  {(void*) "RVMPlugin", (void*)"primitiveGetCore", (void*)primitiveGetCore},
  {(void*) "RVMPlugin", (void*)"primitiveGetCoreIAmRunningOn", (void*)primitiveGetCoreIAmRunningOn},
  {(void*) "RVMPlugin", (void*)"primitiveGetMutability", (void*)primitiveGetMutability},
  {(void*) "RVMPlugin", (void*)"primitiveShuffle", (void*)primitiveShuffle},
  {(void*) "RVMPlugin", (void*)"primitiveSpread", (void*)primitiveSpread},
  {(void*) "RVMPlugin", (void*)"primitiveMoveAllToReadMostlyHeaps", (void*)primitiveMoveAllToReadMostlyHeaps},
  {(void*) "RVMPlugin", (void*)"primitiveTraceCores", (void*)primitiveTraceCores},
  {(void*) "RVMPlugin", (void*)"primitiveTraceMutatedReplicatedObjects", (void*)primitiveTraceMutatedReplicatedObjects},
  {(void*) "RVMPlugin", (void*)"primitivePrintReadWriteReadMostlyBytesUsed", (void*)primitivePrintReadWriteReadMostlyBytesUsed},

  {(void*) "RVMPlugin", (void*)"primitiveAllObjectsInHeap", (void*)primitiveAllObjectsInHeap},
  {(void*) "RVMPlugin", (void*)"primitivePrintStack", (void*)primitivePrintStack},
  {(void*) "RVMPlugin", (void*)"primitivePrintObjectForVMDebugging", (void*) primitivePrintObjectForVMDebugging},
  {(void*) "RVMPlugin", (void*)"primitivePrintExecutionTrace", (void*)primitivePrintExecutionTrace},
  {(void*) "RVMPlugin", (void*)"primitiveThisProcess", (void*)primitiveThisProcess},
  {(void*) "RVMPlugin", (void*)"primitivePrint", (void*)primitivePrint},
  {(void*) "RVMPlugin", (void*)"primitivePrintStats", (void*)primitivePrintStats},
  {(void*) "RVMPlugin", (void*)"primitiveResetPerfCounters", (void*)primitiveResetPerfCounters},
  {(void*) "RVMPlugin", (void*)"primitiveCoreCount", (void*)primitiveCoreCount},
  {(void*) "RVMPlugin", (void*)"primitiveRunningProcessByCore", (void*)primitiveRunningProcessByCore},

  {(void*) "RVMPlugin", (void*)"primitiveGetExtraPreheaderWord", (void*)primitiveGetExtraPreheaderWord},
  {(void*) "RVMPlugin", (void*)"primitiveSetExtraPreheaderWord", (void*)primitiveSetExtraPreheaderWord},
  {(void*) "RVMPlugin", (void*)"primitiveSetExtraWordSelector", (void*)primitiveSetExtraWordSelector},

  {(void*) "RVMPlugin", (void*)"primitiveWriteSnapshot", (void*)primitiveWriteSnapshot},

  {(void*) "RVMPlugin", (void*)"primitiveEmergencySemaphore", (void*)primitiveEmergencySemaphore},
  {(void*) "RVMPlugin", (void*)"primitiveMicrosecondClock", (void*)primitiveMicrosecondClock},
  {(void*) "RVMPlugin", (void*)"primitiveCycleCounter", (void*)primitiveCycleCounter},
  
  {(void*) "RVMPlugin", (void*)"primitiveUseCPUTime", (void*)primitiveUseCPUTime},

 
  {(void*) "RVMPlugin", (void*)"setInterpreter", (void*)setInterpreter},
  
  {(void*) "RVMPlugin", (void*)"primitiveRunsHeadless", (void*)primitiveRunsHeadless},

  {NULL, NULL, NULL}
};


void ioProcessEvents_wrapper() {
  static bool recurse = false;  // threadsafe, this function is exclusivly executed on the main core, Stefan 2009-09-05 (hope there is not reflective call by name anywhere else...)
  if (recurse) return;
  recurse = true;
  ioProcessEvents();
  recurse = false;
}

