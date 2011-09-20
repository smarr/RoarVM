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


# define INIT_BROADCAST(REAL_T,BROADCAST_T,name, init_val) _##name = init_val;
# define INIT_FORMERLY_BROADCAST(REAL_T,BROADCAST_T,name, init_val) _##name = init_val;
# define INIT_SHARED_MEMORY_VARS(REAL_T,BROADCAST_T,name, init_val) shared_memory_fields->_##name = init_val;


Squeak_Interpreter::Squeak_Interpreter() 
#if !On_Tilera
: _my_rank(Logical_Core::my_rank()), _my_core(Logical_Core::my_core()) 
#endif
{
  bcCount = 1;
  remapBufferCount = 0;
  mutated_read_mostly_objects_count = 0;
  yieldCount = 0;
  interruptCheckCount = unforcedInterruptCheckCount = 0;
  update_times_when_yielding();
  update_times_when_resuming();
  cyclesRunning =    cyclesWaiting = 0;
  numberOfMovedMutatedRead_MostlyObjects = 0;
  cyclesMovingMutatedRead_MostlyObjects = 0;
  emergency_semaphore_signal_requested = false;

  static int dummy = 17;
  global_sequence_number = print_sequence_number = &dummy;
  running_process_by_core = NULL;

// Not used, but left in for debugging
/*static bool db = false;
  debug_flag = &db;
  static int dbi = 0;
  debug_int = &dbi;  */


  safepoint_tracker = NULL;
  safepoint_master_control = NULL;
  safepoint_ability = NULL;


  registers_stored();
  uninternalized(); unexternalized();

  added_process_count = 12; // a few for free initiallly

  static Shared_memory_fields dummy_shared;
  shared_memory_fields = &dummy_shared;

  FOR_ALL_BROADCAST(INIT_BROADCAST)
  FOR_ALL_FORMERLY_BROADCAST(INIT_FORMERLY_BROADCAST)
  FOR_ALL_HELD_IN_SHARED_MEMORY(INIT_SHARED_MEMORY_VARS)

  doing_primitiveClosureValueNoContextSwitch = false;
}


void Squeak_Interpreter::init_rank() {
#if !On_Tilera && false
  _my_rank = Logical_Core::my_rank();
  _my_core = Logical_Core::my_core();
#endif  
}

bool Squeak_Interpreter::is_initialized() { return roots.is_initialized(); }

void Squeak_Interpreter::initialize(Oop soo, bool from_checkpoint) {
  if (!from_checkpoint) roots.initialize(soo);

  if (Logical_Core::running_on_main()) {
    
    timeout_deferral_counters = (int32*)Memory_Semantics::shared_calloc(Max_Number_Of_Cores, sizeof(timeout_deferral_counters[0]));
    
    scheduler_mutex.initialize_globals();
    semaphore_mutex.initialize_globals();

    safepoint_tracker = new Safepoint_Tracker();
    safepoint_master_control = new Safepoint_Master_Control();

     global_sequence_number = (int*)OS_Interface::malloc_in_mem(sizeof(int), sizeof(int));
    *global_sequence_number = 0;

     print_sequence_number = (int*)OS_Interface::malloc_in_mem(sizeof(int), sizeof(int));
    *print_sequence_number = 0;

    // STEFAN: looks like it is not used at all
/*     debug_flag = (bool*)OS_Interface::malloc_in_mem(sizeof(bool), sizeof(bool));
    *debug_flag = false;

    debug_int = (int*)OS_Interface::malloc_in_mem(sizeof(int), sizeof(int));
    *debug_int = -1; */

    running_process_by_core            = (Oop*)OS_Interface::malloc_in_mem(sizeof(Oop), Logical_Core::group_size * sizeof(Oop));
    FOR_ALL_RANKS(i)
      running_process_by_core[i] = roots.nilObj;

    shared_memory_fields = (Shared_memory_fields*)OS_Interface::malloc_in_mem(sizeof(long long int), sizeof(Shared_memory_fields));
    bzero(shared_memory_fields, sizeof(*shared_memory_fields));
    FOR_ALL_HELD_IN_SHARED_MEMORY(INIT_SHARED_MEMORY_VARS)
  }


  if (!from_checkpoint) {
    set_activeContext(roots.nilObj);
    set_theHomeContext(roots.nilObj, false);

    set_method(roots.nilObj);
    roots.receiver = roots.nilObj;
    roots.messageSelector = roots.nilObj;
    roots.newMethod = roots.nilObj;
    roots.methodClass = roots.nilObj;
    roots.lkupClass = roots.nilObj;
    roots.receiverClass = roots.nilObj;
    roots.newNativeMethod = roots.nilObj;
    flushInterpreterCaches();
    loadInitialContext();
    initialCleanup();
    interruptCheckCounter = 0;
    interruptChecksEveryNms = 1;
    showSurfaceFn = NULL;

    successFlag = true;

    globalSessionID = 0;


    multicore_interrupt_check = false;
  }
  
# if Dump_Bytecode_Cycles
  bc_cycles_index = 0;
  OS_Interface::get_cycle_count(); // caching?
  u_int64 start_tare = OS_Interface::get_cycle_count();
  u_int64 end_tare = OS_Interface::get_cycle_count();
  bc_cycles_tare = end_tare - start_tare; // meas time
# endif
}


void Squeak_Interpreter::do_all_roots(Oop_Closure* oc) {
  FOR_EACH_ROOT(&roots,oopp) oc->value(oopp, (Object_p)NULL);
  for (int i = 0;  i < remapBufferCount;  ++i)
    oc->value(&remapBuffer[i], (Object_p)NULL);
  for (int i = 0;  i < mutated_read_mostly_objects_count;  ++i)
    oc->value(&mutated_read_mostly_objects[i], (Object_p)NULL);
  if (mutated_read_mostly_object_tracer() != NULL)
    mutated_read_mostly_object_tracer()->do_all_roots(oc);
  if (execution_tracer() != NULL)
    execution_tracer()->do_all_roots(oc);
  if (debugging_tracer() != NULL)
    debugging_tracer()->do_all_roots(oc);
  if (Logical_Core::running_on_main())
    FOR_ALL_RANKS(i)
      oc->value(&running_process_by_core[i], (Object_p)NULL);
  Deferred_Request::do_all_roots(oc);
}

void Squeak_Interpreter::flushInterpreterCaches() {
  methodCache.flush_method_cache();
      atCache.flush_at_cache();
}




void Squeak_Interpreter::loadInitialContext() {
  if (Logical_Core::running_on_main()) {
    Scheduler_Mutex sm("loadInitialContext");
    assert_active_process_not_nil();
    Oop proc = schedulerPointer_obj()->fetchPointer(Object_Indices::ActiveProcessIndex);
    proc.as_object()->add_process_to_scheduler_list(); // unlike squeak rvm keeps running procs in lists
    set_running_process(proc, "loadInitialContext");
    set_activeContext(proc.as_object()->get_suspended_context_of_process_and_mark_running());
    // (activeContext < youngStart) ifTrue: [ self beRootIfOld: activeContext ].
    fetchContextRegisters(activeContext(), activeContext_obj());
  }
  else
    unset_running_process();
  reclaimableContextCount = 0;
}

void Squeak_Interpreter::initialCleanup() {

	// "Images written by VMs earlier than 3.6/3.7 will wrongly have the root bit
  //  set on the active context. Besides clearing the root bit, we treat this
  //  as a marker that these images also lack a cleanup of external primitives
  //  (which has been introduced at the same time when the root bit problem was
  //  fixed). In this case, we merely flush them from here."

	if (!(activeContext_obj()->baseHeader & Object::RootBit)) // "root bit is clean"
    return;

	// "Clean root bit of activeContext"

  activeContext_obj()->baseHeader &= ~Object::RootBit;
	// "Clean external primitives"
  if (Logical_Core::running_on_main())
	   flushExternalPrimitives();
}

void Squeak_Interpreter::flushExternalPrimitives() {
  // "Flush the references to external functions from plugin
  // primitives. This will force a reload of those primitives when
  // accessed next.
  // Note: We must flush the method cache here so that any
  // failed primitives are looked up again."
  //
  // return true iff found a call to primitiveThisProcerss

  The_Memory_System()->flushExternalPrimitives();
  flushInterpreterCachesMessage_class().send_to_all_cores();
  flushObsoleteIndexedPrimitives();
  flushExternalPrimitiveTable();
}

void Squeak_Interpreter::flushObsoleteIndexedPrimitives() {
  //	"Flush the pointers in the obsolete indexed primitive table"
	obsoleteIndexedPrimitiveTable.flush();
}


void Squeak_Interpreter::flushExternalPrimitiveTable() { externalPrimitiveTable()->flush(); }


__attribute__((unused)) static u_char lastBC;
__attribute__((unused)) static int lastBCCount;
void Squeak_Interpreter::interpret() {
  /*
   "This is the main interpreter loop. It normally loops forever,
    fetching and executing bytecodes. When running in the context of a browser
    plugin VM, however, it must return control to the browser periodically.
    This should done only when the state of the currently running Squeak thread
    is safely stored in the object heap. Since this is the case at the moment
    that a check for interrupts is performed, that is when we return to the
    browser if it is time to do so. Interrupt checks happen quite frequently."
   */

  assert_message(Header_Type::Shift == 0  &&  Header_Type::Width >= Tag_Size,
         "lots of code, including Oop packing into class headers, and the mem_bits fns on Oops depends on this");

  Safepoint_Ability sa(false); // about to internalize things
	internalizeIPandSP();
	fetchNextBytecode();
  externalizeIPandSP(); // for assertions in let_one_through

  for (let_one_through();  ; ) {
    check_for_multicore_interrupt();
    if (Collect_Performance_Counters)
      u_int64 start = OS_Interface::get_cycle_count();

    assert(activeContext_obj()->is_read_write());

    if (check_many_assertions) {
      verify_active_context_through_internal_stack_top();
      internalStackTop().verify_oop();
      assert(internalStackTop().is_int() || internalStackTop().as_object()->baseHeader);
      assert((activeContext_obj()->baseHeader & ~0xff));
      Oop s = activeContext_obj()->fetchPointer(Object_Indices::SenderIndex);
      assert(s.is_mem());
      Object_p s_obj = s.as_object();
      assert(s_obj->my_heap_contains_me());

      assert (!roots.freeContexts.is_mem() ||
               roots.freeContexts.as_object()->hasContextHeader());

      if ( check_assertions
      &&   roots.freeContexts != Object::NilContext()) {
        Oop x = roots.freeContexts.as_object()->fetchPointer(Object_Indices::Free_Chain_Index);
        assert (x.is_mem()  ||  x == Object::NilContext());
      }
      assert( internalStackTop().bits() );
    }
    if (check_assertions || CountByteCodesAndStopAt)
      lastBCCount = bcCount;
    if (CountByteCodesAndStopAt ) {
       if (bcCount == CountByteCodesAndStopAt)
         breakpoint();
      if (bcCount >= CountByteCodesAndStopAt ) {
        printBC(currentBytecode, dittoing_stdout_printer);  dittoing_stdout_printer->nl();
      }
    }
    assert_internal();
    if (Trace_Execution  &&  execution_tracer() != NULL)
      execution_tracer()->trace(this);
    assert(get_running_process() != roots.nilObj);

    assert_stored_if_no_proc();
    assert(get_running_process().as_object()->is_process_running());

    assert_method_is_correct(false, "right before dispatch");

    if (Hammer_Safepoints && Logical_Core::running_on_main()) { 
      externalizeIPandSP();
      Safepoint_Ability sa(true);
      while (true) {
        static int n = 0;
        printf("<%d", ++n);
        Safepoint_for_moving_objects sp("test safepoint"); // sends mesgs to other cores to allocate arrays, might cause GC
        printf(">");
      }
     internalizeIPandSP();
    }
    
    if (doing_primitiveClosureValueNoContextSwitch)
      doing_primitiveClosureValueNoContextSwitch = false;
    
#   if Dump_Bytecode_Cycles
    bc_cycles[bc_cycles_index] = OS_Interface::get_cycle_count();
#   endif
    
    dispatch(currentBytecode);
    
#   if Dump_Bytecode_Cycles
    bc_cycles[bc_cycles_index] = OS_Interface::get_cycle_count() - bc_cycles[bc_cycles_index] - bc_cycles_tare;
    ++bc_cycles_index;
    if (bc_cycles_index == sizeof(bc_cycles)/sizeof(bc_cycles[0])) {
      FILE* f = fopen("bc_cycles.txt", "w");
      for (int i = 0;  i < sizeof(bc_cycles)/sizeof(bc_cycles[0]); ++i) fprintf(f, "%lld\n", bc_cycles[i]);
      fclose(f);
      exit(0);
    }
# endif

    assert(!process_is_scheduled_and_executing() || get_running_process().as_object()->is_process_running());

    assert_stored_if_no_proc();

    PERF_CNT(this, add_interpret_cycles(OS_Interface::get_cycle_count() - start));
    
    // for debugging check that the stack is not growing to big
    if (Include_Debugging_Code && (count_stack_depth() > 1000)) {
      OS_Interface::breakpoint();
    }
    
  }
  internal_undo_prefetch();
	externalizeIPandSP();
}



void Squeak_Interpreter::let_one_through() {
  const int winner = Logical_Core::main_rank;
  const int my_rank = this->my_rank();

  if (my_rank != winner) {
    roots.running_process_or_nil = roots.nilObj;
    if (Track_Processes)
      running_process_by_core[my_rank] = roots.running_process_or_nil;

    set_activeContext(roots.nilObj);
    registers_stored();
    internalized(); // for first externalize in multicore_interrupt
    unexternalized();
    update_times_when_yielding();
   }
  update_times_when_resuming();
  multicore_interrupt_check = true;
  addedScheduledProcessMessage_class().send_to_other_cores(); // one is enough for now; all tiles will wake up
}



void Squeak_Interpreter::update_times_when_resuming() {
  cycles_at_resume = OS_Interface::get_cycle_count();
  cyclesWaiting       +=    cycles_at_resume -    cycles_at_yield;
  I_am_running = true;
}

void Squeak_Interpreter::update_times_when_yielding() {
  ++yieldCount;
  cycles_at_yield = OS_Interface::get_cycle_count();
  cyclesRunning       +=    cycles_at_yield -    cycles_at_resume;
  I_am_running = false;
}

void Squeak_Interpreter::update_times_when_asking() {
  if (I_am_running)  update_times_when_yielding();
  else               update_times_when_resuming();
}



void Squeak_Interpreter::dispatch(u_char currentByte) {
  if (Check_Prefetch)  have_executed_currentBytecode = true;
  switch (currentByte) {
  case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7: case 8: case 9:
  case 10: case 11: case 12: case 13: case 14: case 15:
    pushReceiverVariableBytecode(); break;
  case 16: case 17: case 18: case 19:
  case 20: case 21: case 22: case 23: case 24: case 25: case 26: case 27: case 28: case 29:
  case 30: case 31:
    pushTemporaryVariableBytecode(); break;
  case 32: case 33: case 34: case 35: case 36: case 37: case 38: case 39:
  case 40: case 41: case 42: case 43: case 44: case 45: case 46: case 47: case 48: case 49:
  case 50: case 51: case 52: case 53: case 54: case 55: case 56: case 57: case 58: case 59:
  case 60: case 61: case 62: case 63:
    pushLiteralConstantBytecode(); break;
  case 64: case 65: case 66: case 67: case 68: case 69:
  case 70: case 71: case 72: case 73: case 74: case 75: case 76: case 77: case 78: case 79:
  case 80: case 81: case 82: case 83: case 84: case 85: case 86: case 87: case 88: case 89:
  case 90: case 91: case 92: case 93: case 94: case 95:
    pushLiteralVariableBytecode(); break;
  case 96: case 97: case 98: case 99:
  case 100: case 101: case 102: case 103:
    storeAndPopReceiverVariableBytecode(); break;
  case 104: case 105: case 106: case 107: case 108: case 109:
  case 110: case 111:
    storeAndPopTemporaryVariableBytecode(); break;
  case 112: pushReceiverBytecode(); break;
  case 113: pushConstantTrueBytecode(); break;
  case 114: pushConstantFalseBytecode(); break;
  case 115: pushConstantNilBytecode(); break;
  case 116: pushConstantMinusOneBytecode(); break;
  case 117: pushConstantZeroBytecode(); break;
  case 118: pushConstantOneBytecode(); break;
  case 119: pushConstantTwoBytecode(); break;
  case 120: returnReceiver(); break;
  case 121: returnTrue(); break;
  case 122: returnFalse(); break;
  case 123: returnNil(); break;
  case 124: returnTopFromMethod(); break;
  case 125: returnTopFromBlock(); break;
  case 126: unknownBytecode(); break;
  case 127: unknownBytecode(); break;
  case 128: extendedPushBytecode(); break;
  case 129: extendedStoreBytecode(); break;
  case 130: extendedStoreAndPopBytecode(); break;
  case 131: singleExtendedSendBytecode(); break;
  case 132: doubleExtendedDoAnythingBytecode(); break;
  case 133: singleExtendedSuperBytecode(); break;
  case 134: secondExtendedSendBytecode(); break;
  case 135: popStackBytecode(); break;
  case 136: duplicateTopBytecode(); break;
  case 137: pushActiveContextBytecode(); break;
      
# if Include_Closure_Support
    case 138: pushNewArrayBytecode(); break;
    case 139: unknownBytecode(); break;
    case 140: pushRemoteTempLongBytecode(); break;
    case 141: storeRemoteTempLongBytecode(); break;
    case 142: storeAndPopRemoteTempLongBytecode(); break;
    case 143: pushClosureCopyCopiedValuesBytecode(); break;
# else
  case 138: case 139: case 140: case 141: case 142: case 143:
    experimentalBytecode(); break;
# endif
      
  case 144: case 145: case 146: case 147: case 148: case 149:
  case 150: case 151:
    shortUnconditionalJump(); break;
  case 152: case 153: case 154: case 155: case 156: case 157: case 158: case 159:
    shortConditionalJump(); break;
  case 160: case 161: case 162: case 163: case 164: case 165: case 166: case 167:
    longUnconditionalJump(); break;
  case 168: case 169: case 170: case 171:
    longJumpIfTrue(); break;
  case 172: case 173: case 174: case 175:
    longJumpIfFalse(); break;

  // sendArithmeticSelector
  case 176: bytecodePrimAdd(); break;
  case 177: bytecodePrimSubtract(); break;
  case 178: bytecodePrimLessThan(); break;
  case 179: bytecodePrimGreaterThan(); break;
  case 180: bytecodePrimLessOrEqual(); break;
  case 181: bytecodePrimGreaterOrEqual(); break;
  case 182: bytecodePrimEqual(); break;
  case 183: bytecodePrimNotEqual(); break;
  case 184: bytecodePrimMultiply(); break;
  case 185: bytecodePrimDivide(); break;
  case 186: bytecodePrimMod(); break;
  case 187: bytecodePrimMakePoint(); break;
  case 188: bytecodePrimBitShift(); break;
  case 189: bytecodePrimDiv(); break;
  case 190: bytecodePrimBitAnd(); break;
  case 191: bytecodePrimBitOr(); break;

  // sendCommonSelector
  case 192: bytecodePrimAt(); break;
  case 193: bytecodePrimAtPut(); break;
  case 194: bytecodePrimSize(); break;
  case 195: bytecodePrimNext(); break;
  case 196: bytecodePrimNextPut(); break;
  case 197: bytecodePrimAtEnd(); break;
  case 198: bytecodePrimEquivalent(); break;
  case 199: bytecodePrimClass(); break;
  case 200: bytecodePrimBlockCopy(); break;
  case 201: bytecodePrimValue(); break;
  case 202: bytecodePrimValueWithArg(); break;
  case 203: bytecodePrimDo(); break;
  case 204: bytecodePrimNew(); break;
  case 205: bytecodePrimNewWithArg(); break;
  case 206: bytecodePrimPointX(); break;
  case 207: bytecodePrimPointY(); break;

  case 208: case 209:
  case 210: case 211: case 212: case 213: case 214: case 215: case 216: case 217: case 218: case 219:
  case 220: case 221: case 222: case 223: case 224: case 225: case 226: case 227: case 228: case 229:
  case 230: case 231: case 232: case 233: case 234: case 235: case 236: case 237: case 238: case 239:
  case 240: case 241: case 242: case 243: case 244: case 245: case 246: case 247: case 248: case 249:
  case 250: case 251: case 252: case 253: case 254: case 255:
    sendLiteralSelectorBytecode(); break;
  }
}

void Squeak_Interpreter::printBC(u_char bc, Printer* p) {
  p->printf("on %d: %d: %s", my_rank(), bcCount, bytecode_name(bc));
}

const char* Squeak_Interpreter::bytecode_name(u_char bc) {
  const char* c = "";
    switch (bc) {
      case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7: case 8: case 9:
      case 10: case 11: case 12: case 13: case 14: case 15:
        c = "pushReceiverVariableBytecode"; break;
      case 16: case 17: case 18: case 19:
      case 20: case 21: case 22: case 23: case 24: case 25: case 26: case 27: case 28: case 29:
      case 30: case 31:
        c = "pushTemporaryVariableBytecode"; break;
      case 32: case 33: case 34: case 35: case 36: case 37: case 38: case 39:
      case 40: case 41: case 42: case 43: case 44: case 45: case 46: case 47: case 48: case 49:
      case 50: case 51: case 52: case 53: case 54: case 55: case 56: case 57: case 58: case 59:
      case 60: case 61: case 62: case 63:
        c = "pushLiteralConstantBytecode"; break;
      case 64: case 65: case 66: case 67: case 68: case 69:
      case 70: case 71: case 72: case 73: case 74: case 75: case 76: case 77: case 78: case 79:
      case 80: case 81: case 82: case 83: case 84: case 85: case 86: case 87: case 88: case 89:
      case 90: case 91: case 92: case 93: case 94: case 95:
        c = "pushLiteralVariableBytecode"; break;
      case 96: case 97: case 98: case 99:
      case 100: case 101: case 102: case 103:
        c = "storeAndPopReceiverVariableBytecode"; break;
      case 104: case 105: case 106: case 107: case 108: case 109:
      case 110: case 111:
        c = "storeAndPopTemporaryVariableBytecode"; break;
      case 112: c = "pushReceiverBytecode"; break;
      case 113: c = "pushConstantTrueBytecode"; break;
      case 114: c = "pushConstantFalseBytecode"; break;
      case 115: c = "pushConstantNilBytecode"; break;
      case 116: c = "pushConstantMinusOneBytecode"; break;
      case 117: c = "pushConstantZeroBytecode"; break;
      case 118: c = "pushConstantOneBytecode"; break;
      case 119: c = "pushConstantTwoBytecode"; break;
      case 120: c = "returnReceiver"; break;
      case 121: c = "returnTrue"; break;
      case 122: c = "returnFalse"; break;
      case 123: c = "returnNil"; break;
      case 124: c = "returnTopFromMethod"; break;
      case 125: c = "returnTopFromBlock"; break;
      case 126: c = "unknownBytecode"; break;
      case 127: c = "unknownBytecode"; break;
      case 128: c = "extendedPushBytecode"; break;
      case 129: c = "extendedStoreBytecode"; break;
      case 130: c = "extendedStoreAndPopBytecode"; break;
      case 131: c = "singleExtendedSendBytecode"; break;
      case 132: c = "doubleExtendedDoAnythingBytecode"; break;
      case 133: c = "singleExtendedSuperBytecode"; break;
      case 134: c = "secondExtendedSendBytecode"; break;
      case 135: c = "popStackBytecode"; break;
      case 136: c = "duplicateTopBytecode"; break;
      case 137: c = "pushActiveContextBytecode"; break;

# if Include_Closure_Support
      case 138: c = "pushNewArrayBytecode"; break;
      case 139: c = "unknownBytecode"; break;
      case 140: c = "pushRemoteTempLongBytecode"; break;
      case 141: c = "storeRemoteTempLongBytecode"; break;
      case 142: c = "storeAndPopRemoteTempLongBytecode"; break;
      case 143: c = "pushClosureCopyCopiedValuesBytecode"; break;
# else
      case 138: case 139: case 140: case 141: case 142: case 143:
        c = "experimentalBytecode"; break;
# endif

      case 144: case 145: case 146: case 147: case 148: case 149:
      case 150: case 151:
        c = "shortUnconditionalJump"; break;
      case 152: case 153: case 154: case 155: case 156: case 157: case 158: case 159:
        c = "shortConditionalJump"; break;
      case 160: case 161: case 162: case 163: case 164: case 165: case 166: case 167:
        c = "longUnconditionalJump"; break;
      case 168: case 169: case 170: case 171:
        c = "longJumpIfTrue"; break;
      case 172: case 173: case 174: case 175:
        c = "longJumpIfFalse"; break;

        // sendArithmeticSelector
      case 176: c = "bytecodePrimAdd"; break;
      case 177: c = "bytecodePrimSubtract"; break;
      case 178: c = "bytecodePrimLessThan"; break;
      case 179: c = "bytecodePrimGreaterThan"; break;
      case 180: c = "bytecodePrimLessOrEqual"; break;
      case 181: c = "bytecodePrimGreaterOrEqual"; break;
      case 182: c = "bytecodePrimEqual"; break;
      case 183: c = "bytecodePrimNotEqual"; break;
      case 184: c = "bytecodePrimMultiply"; break;
      case 185: c = "bytecodePrimDivide"; break;
      case 186: c = "bytecodePrimMod"; break;
      case 187: c = "bytecodePrimMakePoint"; break;
      case 188: c = "bytecodePrimBitShift"; break;
      case 189: c = "bytecodePrimDiv"; break;
      case 190: c = "bytecodePrimBitAnd"; break;
      case 191: c = "bytecodePrimBitOr"; break;

        // sendCommonSelector
      case 192: c = "bytecodePrimAt"; break;
      case 193: c = "bytecodePrimAtPut"; break;
      case 194: c = "bytecodePrimSize"; break;
      case 195: c = "bytecodePrimNext"; break;
      case 196: c = "bytecodePrimNextPut"; break;
      case 197: c = "bytecodePrimAtEnd"; break;
      case 198: c = "bytecodePrimEquivalent"; break;
      case 199: c = "bytecodePrimClass"; break;
      case 200: c = "bytecodePrimBlockCopy"; break;
      case 201: c = "bytecodePrimValue"; break;
      case 202: c = "bytecodePrimValueWithArg"; break;
      case 203: c = "bytecodePrimDo"; break;
      case 204: c = "bytecodePrimNew"; break;
      case 205: c = "bytecodePrimNewWithArg"; break;
      case 206: c = "bytecodePrimPointX"; break;
      case 207: c = "bytecodePrimPointY"; break;

      case 208: case 209:
      case 210: case 211: case 212: case 213: case 214: case 215: case 216: case 217: case 218: case 219:
      case 220: case 221: case 222: case 223: case 224: case 225: case 226: case 227: case 228: case 229:
      case 230: case 231: case 232: case 233: case 234: case 235: case 236: case 237: case 238: case 239:
      case 240: case 241: case 242: case 243: case 244: case 245: case 246: case 247: case 248: case 249:
      case 250: case 251: case 252: case 253: case 254: case 255:
        c = "sendLiteralSelectorBytecode"; break;
  }
  return c;
}


int Squeak_Interpreter::literal_index_of_bytecode(u_char* bcp) {
  int c = -1;
  switch (*bcp) {
    case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7: case 8: case 9:
    case 10: case 11: case 12: case 13: case 14: case 15:
      c = pushReceiverVariableBytecode_literal_index(bcp); break;
    case 16: case 17: case 18: case 19:
    case 20: case 21: case 22: case 23: case 24: case 25: case 26: case 27: case 28: case 29:
    case 30: case 31:
      c = pushTemporaryVariableBytecode_literal_index(bcp); break;
    case 32: case 33: case 34: case 35: case 36: case 37: case 38: case 39:
    case 40: case 41: case 42: case 43: case 44: case 45: case 46: case 47: case 48: case 49:
    case 50: case 51: case 52: case 53: case 54: case 55: case 56: case 57: case 58: case 59:
    case 60: case 61: case 62: case 63:
      c = pushLiteralConstantBytecode_literal_index(bcp); break;
    case 64: case 65: case 66: case 67: case 68: case 69:
    case 70: case 71: case 72: case 73: case 74: case 75: case 76: case 77: case 78: case 79:
    case 80: case 81: case 82: case 83: case 84: case 85: case 86: case 87: case 88: case 89:
    case 90: case 91: case 92: case 93: case 94: case 95:
      c = pushLiteralVariableBytecode_literal_index(bcp); break;
    case 96: case 97: case 98: case 99:
    case 100: case 101: case 102: case 103:
      c = storeAndPopReceiverVariableBytecode_literal_index(bcp); break;
    case 104: case 105: case 106: case 107: case 108: case 109:
    case 110: case 111:
      c = storeAndPopTemporaryVariableBytecode_literal_index(bcp); break;
    case 112: c = pushReceiverBytecode_literal_index(bcp); break;
    case 113: c = pushConstantTrueBytecode_literal_index(bcp); break;
    case 114: c = pushConstantFalseBytecode_literal_index(bcp); break;
    case 115: c = pushConstantNilBytecode_literal_index(bcp); break;
    case 116: c = pushConstantMinusOneBytecode_literal_index(bcp); break;
    case 117: c = pushConstantZeroBytecode_literal_index(bcp); break;
    case 118: c = pushConstantOneBytecode_literal_index(bcp); break;
    case 119: c = pushConstantTwoBytecode_literal_index(bcp); break;
    case 120: c = returnReceiver_literal_index(bcp); break;
    case 121: c = returnTrue_literal_index(bcp); break;
    case 122: c = returnFalse_literal_index(bcp); break;
    case 123: c = returnNil_literal_index(bcp); break;
    case 124: c = returnTopFromMethod_literal_index(bcp); break;
    case 125: c = returnTopFromBlock_literal_index(bcp); break;
    case 126: c = unknownBytecode_literal_index(bcp); break;
    case 127: c = unknownBytecode_literal_index(bcp); break;
    case 128: c = extendedPushBytecode_literal_index(bcp); break;
    case 129: c = extendedStoreBytecode_literal_index(bcp); break;
    case 130: c = extendedStoreAndPopBytecode_literal_index(bcp); break;
    case 131: c = singleExtendedSendBytecode_literal_index(bcp); break;
    case 132: c = doubleExtendedDoAnythingBytecode_literal_index(bcp); break;
    case 133: c = singleExtendedSuperBytecode_literal_index(bcp); break;
    case 134: c = secondExtendedSendBytecode_literal_index(bcp); break;
    case 135: c = popStackBytecode_literal_index(bcp); break;
    case 136: c = duplicateTopBytecode_literal_index(bcp); break;
    case 137: c = pushActiveContextBytecode_literal_index(bcp); break;

# if Include_Closure_Support
    case 138: c = pushNewArrayBytecode_literal_index(bcp); break;
    case 139: c = unknownBytecode_literal_index(bcp); break;
    case 140: c = pushRemoteTempLongBytecode_literal_index(bcp); break;
    case 141: c = storeRemoteTempLongBytecode_literal_index(bcp); break;
    case 142: c = storeAndPopRemoteTempLongBytecode_literal_index(bcp); break;
    case 143: c = pushClosureCopyCopiedValuesBytecode_literal_index(bcp); break;
# else
    case 138: case 139: case 140: case 141: case 142: case 143:
      c = experimentalBytecode_literal_index(bcp); break;
# endif

    case 144: case 145: case 146: case 147: case 148: case 149:
    case 150: case 151:
      c = shortUnconditionalJump_literal_index(bcp); break;
    case 152: case 153: case 154: case 155: case 156: case 157: case 158: case 159:
      c = shortConditionalJump_literal_index(bcp); break;
    case 160: case 161: case 162: case 163: case 164: case 165: case 166: case 167:
      c = longUnconditionalJump_literal_index(bcp); break;
    case 168: case 169: case 170: case 171:
      c = longJumpIfTrue_literal_index(bcp); break;
    case 172: case 173: case 174: case 175:
      c = longJumpIfFalse_literal_index(bcp); break;

      // sendArithmeticSelector
    case 176: c = bytecodePrimAdd_literal_index(bcp); break;
    case 177: c = bytecodePrimSubtract_literal_index(bcp); break;
    case 178: c = bytecodePrimLessThan_literal_index(bcp); break;
    case 179: c = bytecodePrimGreaterThan_literal_index(bcp); break;
    case 180: c = bytecodePrimLessOrEqual_literal_index(bcp); break;
    case 181: c = bytecodePrimGreaterOrEqual_literal_index(bcp); break;
    case 182: c = bytecodePrimEqual_literal_index(bcp); break;
    case 183: c = bytecodePrimNotEqual_literal_index(bcp); break;
    case 184: c = bytecodePrimMultiply_literal_index(bcp); break;
    case 185: c = bytecodePrimDivide_literal_index(bcp); break;
    case 186: c = bytecodePrimMod_literal_index(bcp); break;
    case 187: c = bytecodePrimMakePoint_literal_index(bcp); break;
    case 188: c = bytecodePrimBitShift_literal_index(bcp); break;
    case 189: c = bytecodePrimDiv_literal_index(bcp); break;
    case 190: c = bytecodePrimBitAnd_literal_index(bcp); break;
    case 191: c = bytecodePrimBitOr_literal_index(bcp); break;

      // sendCommonSelector
    case 192: c = bytecodePrimAt_literal_index(bcp); break;
    case 193: c = bytecodePrimAtPut_literal_index(bcp); break;
    case 194: c = bytecodePrimSize_literal_index(bcp); break;
    case 195: c = bytecodePrimNext_literal_index(bcp); break;
    case 196: c = bytecodePrimNextPut_literal_index(bcp); break;
    case 197: c = bytecodePrimAtEnd_literal_index(bcp); break;
    case 198: c = bytecodePrimEquivalent_literal_index(bcp); break;
    case 199: c = bytecodePrimClass_literal_index(bcp); break;
    case 200: c = bytecodePrimBlockCopy_literal_index(bcp); break;
    case 201: c = bytecodePrimValue_literal_index(bcp); break;
    case 202: c = bytecodePrimValueWithArg_literal_index(bcp); break;
    case 203: c = bytecodePrimDo_literal_index(bcp); break;
    case 204: c = bytecodePrimNew_literal_index(bcp); break;
    case 205: c = bytecodePrimNewWithArg_literal_index(bcp); break;
    case 206: c = bytecodePrimPointX_literal_index(bcp); break;
    case 207: c = bytecodePrimPointY_literal_index(bcp); break;

    case 208: case 209:
    case 210: case 211: case 212: case 213: case 214: case 215: case 216: case 217: case 218: case 219:
    case 220: case 221: case 222: case 223: case 224: case 225: case 226: case 227: case 228: case 229:
    case 230: case 231: case 232: case 233: case 234: case 235: case 236: case 237: case 238: case 239:
    case 240: case 241: case 242: case 243: case 244: case 245: case 246: case 247: case 248: case 249:
    case 250: case 251: case 252: case 253: case 254: case 255:
      c = sendLiteralSelectorBytecode_literal_index(bcp); break;
  }
  return c;
}








void Squeak_Interpreter::findNewMethodInClass(Oop klass) {
  /*
   "Find the compiled method to be run when the current
   roots.messageSelector is sent to the given class, setting the values
   of 'roots.newMethod' and 'primitiveIndex'.
   */
  if (check_many_assertions) klass.verify_oop();

  if (!lookupInMethodCacheSel(roots.messageSelector, klass)) {
    // "entry was not found in the cache; look it up the hard way"
    lookupMethodInClass(klass);
    roots.lkupClass = klass;
    addNewMethodToCache();
  }
}



Oop Squeak_Interpreter::lookupMethodInClass(Oop lkupClass) {
  assert(safepoint_ability->is_able()); // need to be able to allocate message object without deadlock
  Object_p currentClass_obj = (Object_p)NULL;
  for (  Oop currentClass = lkupClass;
         currentClass != roots.nilObj;
       currentClass = currentClass_obj->superclass()) {

    assert(currentClass.verify_oop());
    currentClass_obj = currentClass.as_object();
    Oop dictionary = currentClass_obj->fetchPointer(Object_Indices::MessageDictionaryIndex);
    if (dictionary == roots.nilObj) {
      // "MethodDict pointer is nil (hopefully due a swapped out stub) -- raise exception #cannotInterpret:."
      pushRemappableOop(currentClass); // GC
      createActualMessageTo(lkupClass);
      currentClass_obj = popRemappableOop().as_object();
      roots.messageSelector = splObj(Special_Indices::SelectorCannotInterpret);
      return lookupMethodInClass(currentClass_obj->superclass());
    }
    if (lookupMethodInDictionary(dictionary)) {
      return roots.methodClass = currentClass;
    }
  }
  // Could not find #doesNotUnderstand: -- fatal error
  if ( roots.messageSelector == splObj(Special_Indices::SelectorDoesNotUnderstand)) {

    error_printer->printf("Recursive doesNotUnderstand receiver: ");
    roots.receiver.print(error_printer);
    error_printer->printf("  Lookup class: ");
    roots.lkupClass.print(error_printer);
    error_printer->printf("  Selector: ");
    roots.dnuSelector.print(error_printer);
    error_printer->printf("\n");

    fatal("Recursive not understood error encountered");
  }

  // send doesNotUnderstand

  if ( !roots.messageSelector.as_object()->equals_string("paragraph")
    && !roots.messageSelector.as_object()->equals_string("gradientWindowLook")
    && !roots.messageSelector.as_object()->equals_string("uniformWindowColors")
    && !roots.messageSelector.as_object()->equals_string("externalServerDefsOnly")) {// xxx_dmu

    const int enough_already = 7;
    if (dnu_kvetch_count() < enough_already) {
      print_time();
      dittoing_stdout_printer->printf("%d: sending doesNotUnderstand: ", my_rank());
      roots.messageSelector.print(dittoing_stdout_printer);
      dittoing_stdout_printer->printf(" for object of ");
      currentClass_obj->className().as_object()->print_bytes(dittoing_stdout_printer);
      dittoing_stdout_printer->nl(); // xxx_dmu
      set_dnu_kvetch_count(dnu_kvetch_count() + 1);
      if (dnu_kvetch_count() >= enough_already) lprintf("Enough already! No more kvetching!");
      breakpoint();
    }
    roots.dnuSelector = roots.messageSelector;
    if (check_assertions && !roots.messageSelector.isBytes()) {
      lprintf("sel not bytes\n");
      fatal("Message selector not isBytes");
    }

  } // xxx_dmu
  pushRemappableOop(lkupClass);
  createActualMessageTo(lkupClass);
  roots.messageSelector = splObj(Special_Indices::SelectorDoesNotUnderstand);
  return lookupMethodInClass(popRemappableOop());
}

#if Extra_Preheader_Word_Experiment
Oop Squeak_Interpreter::modify_send_for_preheader_word(Oop rcvr) {
  externalizeIPandSP();
  {
    Safepoint_Ability sa(true);
    pushRemappableOop(rcvr);
    createActualMessageTo(rcvr);
    rcvr = popRemappableOop();      
  }
  internalizeIPandSP();
  
  roots.messageSelector =  roots.extra_preheader_word_selector;

  Oop new_rcvr = Oop::from_bits(rcvr.as_object()->get_extra_preheader_word());
  DEBUG_STORE_CHECK(&localSP()[-get_argumentCount()], new_rcvr);
  localSP()[-get_argumentCount()] = new_rcvr;
  
  return new_rcvr;
}
#endif



bool Squeak_Interpreter::balancedStackAfterPrimitive(int delta, int primIdx, int nArgs, Oop pre_prim_active_context) {
  // Return true if the stack is still balanced after executing primitive
  // primIndex with nArgs args. Delta is 'stackPointer - activeContext' which
  // is a relative measure for the stack pointer
  // (so we don't have to relocate it during the primitive)
  if (!process_is_scheduled_and_executing())
    return true; // prim put last executing process to sleep
  
  if (activeContext() != pre_prim_active_context)
    return true; // prim changed process or context
  
  switch (primIdx) {
    default: break;
    case 81: case 82: case 83: case 84: case 85: case 86: case 87: case 88:
    case 167: case 188: case 189: case 205: case 206: case 221: case 222:
      return true;  // control prims that may unbal the stack
  }
  if (successFlag) {
    // must have nArgs popped off
    if  ( stackPointer() - activeContext_obj()->as_oop_p() + nArgs ==  delta ) return true;
    lprintf("balancedStackAfterPrimitive failed: stackPointer 0x%x, activeContext_obj() 0x%x, nArgs %d, delta %d\n",
            _stackPointer, (Object*)activeContext_obj(), nArgs, delta);
    return false;
  }
  // failed prim leaves stack intact
  return stackPointer() - activeContext_obj()->as_oop_p() == delta;
};

void Squeak_Interpreter::printUnbalancedStack(int primIdx, fn_t fn) {
  lprintf("printUnbalancedStack primitive: %d 0x%x\n", primIdx, fn);
  print_stack_trace(dittoing_stdout_printer);
  unimplemented();
}


void Squeak_Interpreter::internalActivateNewMethod() {
  oop_int_t methodHeader = newMethod_obj()->methodHeader();
  bool needsLarge = methodHeader & Object::LargeContextBit;
  Object_p nco;  Oop newContext;

  if (!needsLarge &&  roots.freeContexts != Object::NilContext()) {
    newContext = roots.freeContexts;
    nco = newContext.as_object();
    assert(nco->my_heap_contains_me());
    Oop nfc = nco->fetchPointer(Object_Indices::Free_Chain_Index);
    assert(nfc == Object::NilContext()  ||  (nfc.is_mem() && nfc.as_object()->headerType() == Header_Type::Short));
    roots.freeContexts = nfc;
  }
  else {
    externalizeIPandSP();
    {
      Safepoint_Ability sa(true);
      nco = allocateOrRecycleContext(needsLarge);
    }
    newContext = nco->as_oop();
    internalizeIPandSP();
    assert(nco->my_heap_contains_me());
  }

  if ( check_many_assertions
      &&  nco->get_count_of_blocks_homed_to_this_method_ctx() > 0
      &&  nco->fetchPointer(Object_Indices::MethodIndex) != roots.newMethod) {
    lprintf("int act changing method from 0x%x to 0x%x in 0x%x, home_flag %d\n",
            nco->fetchPointer(Object_Indices::MethodIndex).bits(),
            roots.newMethod.bits(), nco->as_oop().bits(),
            nco->get_count_of_blocks_homed_to_this_method_ctx());
    lprintf("needsLarge %d\n", needsLarge);
  }

  int tempCount = Object::temporaryCountOfHeader(methodHeader);
  assert(nco->my_heap_contains_me());

  // assume newContext will be recorded as a root if need be by
  // the call to newActiveContext so use unchecked stores

  Oop* where = nco->as_oop_p() + Object::BaseHeaderSize/sizeof(oop_int_t);
  
  DEBUG_STORE_CHECK(&where[Object_Indices::SenderIndex], activeContext());
  where[Object_Indices::SenderIndex] = activeContext();

  Oop contents = Oop::from_int(
                                                                 ((Object_Indices::LiteralStart + Object::literalCountOfHeader(methodHeader)) * bytesPerWord) + 1);
  DEBUG_STORE_CHECK(&where[Object_Indices::InstructionPointerIndex], contents);
  where[Object_Indices::InstructionPointerIndex] = contents;
  
  DEBUG_STORE_CHECK(&where[Object_Indices::StackPointerIndex], Oop::from_int(tempCount));
  where[Object_Indices::StackPointerIndex] = Oop::from_int(tempCount);
  
  DEBUG_STORE_CHECK(&where[Object_Indices::MethodIndex], roots.newMethod);  
  where[Object_Indices::MethodIndex] = roots.newMethod;
  
# if Include_Closure_Support
  DEBUG_STORE_CHECK(&where[Object_Indices::ClosureIndex], roots.nilObj);  
  where[Object_Indices::ClosureIndex] = roots.nilObj;
# endif
  

  // copy rcvr, args
  int argCount2 = get_argumentCount();
  // Use get_argumentCount() instead of argCount2 to avoid tripping over
  // Tilera -O2 compiler bug. -- dmu 6/17/08
  // Try it with 1.3. dmu 6/23/08
  for (int i = 0;  i <= argCount2/*get_argumentCount()*/;  ++i) {
    DEBUG_STORE_CHECK(&where[Object_Indices::ReceiverIndex + i], internalStackValue(argCount2 - i));  
    where[Object_Indices::ReceiverIndex + i] = internalStackValue(argCount2 - i);
  }

  // clear remaining temps to nil, in case it has been recycled
  Oop nnil = roots.nilObj;
  for (int i = argCount2 + 1 + Object_Indices::ReceiverIndex;
       i <=  tempCount + Object_Indices::ReceiverIndex;
       ++i) {
    DEBUG_STORE_CHECK(&where[i], nnil);
    where[i] = nnil;
  }

  internalPop(argCount2 + 1);
  reclaimableContextCount += 1;
  assert(nco->my_heap_contains_me());
  internalNewActiveContext(newContext, nco);
}


void Squeak_Interpreter::activateNewMethod() {
  oop_int_t methodHeader = newMethod_obj()->methodHeader();
  Object_p nco = allocateOrRecycleContext(methodHeader & Object::LargeContextBit);

  if (check_many_assertions
      && nco->get_count_of_blocks_homed_to_this_method_ctx() > 0  &&  nco->fetchPointer(Object_Indices::MethodIndex) != roots.newMethod)
    lprintf("ext act changing method from 0x%x to 0x%x in 0x%x, home_flag %d\n",
            nco->fetchPointer(Object_Indices::MethodIndex).bits(),
            roots.newMethod.bits(),
            nco->as_oop().bits(),
            nco->get_count_of_blocks_homed_to_this_method_ctx());
  Oop newContext = nco->as_oop();

  int initialIP = (Object_Indices::LiteralStart + Object::literalCountOfHeader(methodHeader)) * bytesPerWord  +  1;
  int tempCount = Object::temporaryCountOfHeader(methodHeader);
  // assume newContext will be recorded as a root if need be by
  // the call to newActiveContext so use unchecked stores
  Oop* where = nco->as_oop_p() + Object::BaseHeaderSize/sizeof(oop_int_t);
  
  
  DEBUG_STORE_CHECK( &where[Object_Indices::SenderIndex], activeContext());
  where[Object_Indices::SenderIndex] = activeContext();
  
  DEBUG_STORE_CHECK( &where[Object_Indices::InstructionPointerIndex], Oop::from_int(initialIP));
  where[Object_Indices::InstructionPointerIndex] = Oop::from_int(initialIP);

  DEBUG_STORE_CHECK( &where[Object_Indices::StackPointerIndex], Oop::from_int(tempCount));
  where[Object_Indices::StackPointerIndex] = Oop::from_int(tempCount);

  DEBUG_STORE_CHECK( &where[Object_Indices::MethodIndex], roots.newMethod);
  where[Object_Indices::MethodIndex] = roots.newMethod;

# if Include_Closure_Support
  DEBUG_STORE_CHECK( &where[Object_Indices::ClosureIndex], roots.nilObj);
  where[Object_Indices::ClosureIndex] = roots.nilObj;
# endif


  // copy rcvr, args
  for (int i = 0;  i <= get_argumentCount();  ++i) {
    DEBUG_STORE_CHECK( &where[Object_Indices::ReceiverIndex + i], stackValue(get_argumentCount() - i));
    where[Object_Indices::ReceiverIndex + i] = stackValue(get_argumentCount() - i);
  }

  // clear remaining temps to nil, in case it has been recycled
  Oop nnil = roots.nilObj;
  for (int i = get_argumentCount() + 1 + Object_Indices::ReceiverIndex;  i <=  tempCount + Object_Indices::ReceiverIndex;  ++i) {
    DEBUG_STORE_CHECK(&where[i], nnil);
    where[i] = nnil;
  }

  pop(get_argumentCount() + 1);
  reclaimableContextCount += 1;
  newActiveContext(newContext, nco);
}

# if Include_Closure_Support

void Squeak_Interpreter::activateNewClosureMethod(Object_p blockClosure_obj, Object_p argumentArray_obj_or_null) {
  assert(blockClosure_obj->verify());
  Oop outerContext = blockClosure_obj->fetchPointer(Object_Indices::ClosureOuterContextIndex);
  Object_p outerContext_obj = outerContext.as_object();
  
  Oop closureMethod = outerContext_obj->fetchPointer(Object_Indices::MethodIndex);
  Object_p closureMethod_obj = closureMethod.as_object();
  oop_int_t methodHeader = closureMethod_obj->methodHeader();
  
  pushRemappableOop(blockClosure_obj->as_oop());
  Object_p newContext_obj = allocateOrRecycleContext(methodHeader & Object::LargeContextBit); 
  Oop newContext = newContext_obj->as_oop(); 
  Oop blockClosure = popRemappableOop();
  blockClosure_obj = blockClosure.as_object();
  outerContext = blockClosure_obj->fetchPointer(Object_Indices::ClosureOuterContextIndex);
  outerContext_obj = outerContext.as_object();
  // Throughout RVM, when implementing standard Squeak VM code, we pretend that GC can alter Oops,
  // but in RVM it can only alter Object*'s, and I bet some of my code makes that assumption. -- dmu 6/10
  
  int numCopied = blockClosure_obj->fetchWordLength() - Object_Indices::ClosureFirstCopiedValueIndex; // should be 0 for nil
  
  // Assume newContext will be recorded as a root if necessary by the call to newActiveContext below, so use unchecked stores
  Oop* where = newContext_obj->as_oop_p() + Object::BaseHeaderSize/sizeof(Oop);
  
  DEBUG_STORE_CHECK( &where[Object_Indices::SenderIndex], activeContext());
  where[Object_Indices::SenderIndex] = activeContext();

  DEBUG_STORE_CHECK( &where[Object_Indices::InstructionPointerIndex], blockClosure_obj->fetchPointer(Object_Indices::ClosureStartPCIndex));
  where[Object_Indices::InstructionPointerIndex] = blockClosure_obj->fetchPointer(Object_Indices::ClosureStartPCIndex);

  DEBUG_STORE_CHECK( &where[Object_Indices::StackPointerIndex], Oop::from_int(get_argumentCount() + numCopied));
  where[Object_Indices::StackPointerIndex] = Oop::from_int(get_argumentCount() + numCopied);

  DEBUG_STORE_CHECK( &where[Object_Indices::MethodIndex], outerContext_obj->fetchPointer(Object_Indices::MethodIndex));
  where[Object_Indices::MethodIndex] = outerContext_obj->fetchPointer(Object_Indices::MethodIndex);

  DEBUG_STORE_CHECK( &where[Object_Indices::ClosureIndex], blockClosure);
  where[Object_Indices::ClosureIndex] = blockClosure;

  DEBUG_STORE_CHECK( &where[Object_Indices::ReceiverIndex], outerContext_obj->fetchPointer(Object_Indices::ReceiverIndex));
  where[Object_Indices::ReceiverIndex] = outerContext_obj->fetchPointer(Object_Indices::ReceiverIndex);
  
  if (argumentArray_obj_or_null == NULL ) {
      // copy args
    for (int i = 1;  i <= get_argumentCount();  ++i) {
        DEBUG_STORE_CHECK( &where[Object_Indices::ReceiverIndex + i], stackValue(get_argumentCount() - i));
        where[Object_Indices::ReceiverIndex + i] = stackValue(get_argumentCount() - i);
    }
  }
  else
    transferFromIndexOfObjectToIndexOfObject(argumentArray_obj_or_null->fetchWordLength(),
                                             0,
                                             argumentArray_obj_or_null,
                                             Object_Indices::TempFrameStart,
                                             newContext_obj);
  
  where = newContext_obj->as_oop_p() + Object::BaseHeaderSize/sizeof(Oop) + (Object_Indices::ReceiverIndex + 1 + get_argumentCount());
  for (int i = 0;  i < numCopied;  ++i )  {
    DEBUG_STORE_CHECK( &where[i], blockClosure_obj->fetchPointer(i + Object_Indices::ClosureFirstCopiedValueIndex));
    where[i] = blockClosure_obj->fetchPointer(i + Object_Indices::ClosureFirstCopiedValueIndex);
  }
  
  pop(get_argumentCount() + 1);
  newActiveContext(newContext, newContext_obj);
}

# endif


void Squeak_Interpreter::signalSemaphoreWithIndex(int index) {
  if (index < 0) return;
  
  oop_int_t& count = semaphoresUseBufferA() ? _semaphoresToSignalCountA : _semaphoresToSignalCountB;
  oop_int_t* semas = semaphoresUseBufferA() ? _semaphoresToSignalA : _semaphoresToSignalB;

  if (count < SemaphoresToSignalSize)
    semas[++count] = index;

  broadcast_int32(&count);
  broadcast_int32(&semas[count]);

  forceInterruptCheck();
}


void Squeak_Interpreter::checkForInterrupts(bool is_safe_to_process_events) {
  if (doing_primitiveClosureValueNoContextSwitch)
    return;
  static bool last_use_cpu_ms = use_cpu_ms();
  bool use_cpu_ms_changed = last_use_cpu_ms != use_cpu_ms();
  last_use_cpu_ms = use_cpu_ms();

  multicore_interrupt_check = true;
  Safepoint_Ability sa(true);
  
  // Mask so same wrapping as primitiveMillisecondClock
  assert_method_is_correct_internalizing(true, "start of checkForInterrupts");
  ++interruptCheckCount;
  int now = ioWhicheverMSecs() & MillisecondClockMask;
  if (!interruptCheckForced()  &&  !use_cpu_ms_changed) {
    ++unforcedInterruptCheckCount;
    // "don't play with the feedback if we forced a check. It only makes life difficult"
    if (now - lastTick()  <  interruptChecksEveryNms)  {
      /*
       "wrapping is not a concern, it'll get caught quickly
       enough. This clause is trying to keep a reasonable
       guess of how many times per 	interruptChecksEveryNms we are calling
       quickCheckForInterrupts. Not sure how effective it really is."
       */
      set_interruptCheckCounterFeedBackReset(interruptCheckCounterFeedBackReset() + 10);
    }
    else if (interruptCheckCounterFeedBackReset() <= 1000) {
      set_interruptCheckCounterFeedBackReset(1000);
    }
    else {
      // too slow to recover: set_interruptCheckCounterFeedBackReset(interruptCheckCounterFeedBackReset() - 12);
      set_interruptCheckCounterFeedBackReset(interruptCheckCounterFeedBackReset() / 2);
    }
  }

  // "reset the interrupt check counter"
  interruptCheckCounter = interruptCheckCounterFeedBackReset();

  The_Memory_System()->handle_low_space_signals();

  if (now < lastTick() ||  use_cpu_ms_changed) {
    // ms clock wrapped so correct the nextPollTick
    set_nextPollTick(nextPollTick() - MillisecondClockMask - 1);
  }
  if (now >= nextPollTick()  &&  is_safe_to_process_events) {
    bool s = successFlag; successFlag = true;
    The_Interactions.run_primitive(Logical_Core::main_rank, (fn_t)ioProcessEvents_wrapper);
    successFlag = s;
    // sets interruptPending if interrupt key pressed
    set_nextPollTick(now + 200);
    /*
     msecs to wait before next call to ioProcessEvents.
     Note that strictly speaking we might need to update
    'now' at this point since ioProcessEvents could take a
     very long time on some platforms"
     */
  }
  if (interruptPending()) {
    set_interruptPending(false);
    signalSema(Special_Indices::TheInterruptSemaphore, "checkForInterrupts 649");
  }
  if (nextWakeupTick() != 0) {
    if (now < lastTick()) {
      /*
       "the clock has wrapped. Subtract the wrap
       interval from nextWakeupTick - this might just
       possibly result in 0. Since this is used as a flag
       value for 'no timer' we do the 0 check above"
       */
      // lprintf("WRAPPED now %d, lastTick %d, nextWakeupTick %d, new nextWakeupTick %d\n",
      //   now, lastTick(), nextWakeupTick(), nextWakeupTick() - MillisecondClockMask - 1);
      set_nextWakeupTick(nextWakeupTick() - MillisecondClockMask - 1);
    }
    if (now >= nextWakeupTick()) {
      set_nextWakeupTick(0);
      // set timer interrupt to 0 for no timer
      // lprintf("signalling next\n");
      signalSema(Special_Indices::TheTimerSemaphore, "checkForInterrupts 664");
    }
  }

  // signal any pending finalizations
  if (pendingFinalizationSignals() > 0) {
    Safepoint_Ability sa(false);

    Oop sema = splObj(Special_Indices::TheFinalizationSemaphore);
    if (sema.fetchClass() == splObj(Special_Indices::ClassSemaphore)) {
      sema.as_object()->synchronousSignal("checkForInterrupts 674");
    }
    set_pendingFinalizationSignals(0);
  }

  // signal all semas in semasToSignal
  if (semaphoresToSignalCountA() > 0  ||  semaphoresToSignalCountB() > 0) {
    signalExternalSemaphores("checkForInter 678");
  }

  set_lastTick(now);
  assert_method_is_correct_internalizing(true, "end of checkForInterrupts");
}


void Squeak_Interpreter::createActualMessageTo(Oop aClass) {
  /*
  "Bundle up the selector, arguments and lookupClass into a Message object.
	In the process it pops the arguments off the stack, and pushes the message object.
	This can then be presented as the argument of e.g. #doesNotUnderstand:.
	ikp 11/20/1999 03:59 -- added hook for external runtime compilers."
   */
	// | argumentArray message lookupClass |
  pushRemappableOop(aClass);
  Object_p argumentArray_obj = splObj_obj(Special_Indices::ClassArray)->instantiateClass(get_argumentCount());
	Oop argumentArray = argumentArray_obj->as_oop();
  // "remap argumentArray in case GC happens during allocation"
	pushRemappableOop(argumentArray);
	Object_p message_obj = splObj_obj(Special_Indices::ClassMessage)->instantiateClass(0);
  Oop message = message_obj->as_oop();
	argumentArray = popRemappableOop(); argumentArray_obj = argumentArray.as_object();
	Oop lookupClass = popRemappableOop();
  oopcpy_no_store_check(argumentArray_obj->as_oop_p() + sizeof(argumentArray_obj->baseHeader)/sizeof(Oop),
         stackPointer() - (get_argumentCount() - 1),
         get_argumentCount(),
         argumentArray_obj);
	argumentArray_obj->beRootIfOld();
  popThenPush(get_argumentCount(), message);
	set_argumentCount(1);
  message_obj->storePointer(Object_Indices::MessageSelectorIndex,  roots.messageSelector);
  message_obj->storePointer(Object_Indices::MessageArgumentsIndex, argumentArray);

  // "Only store lookupClass if message has 3 fields (old images don't)"
  if (message_obj->lastPointer()
      >=  (int)(Object_Indices::MessageLookupClassIndex * sizeof(Oop) + sizeof(message_obj->baseHeader)))
    message_obj->storePointer(Object_Indices::MessageLookupClassIndex, lookupClass);
}



void Squeak_Interpreter::transfer_to_highest_priority(const char* why) {
  Scheduler_Mutex sm("transfer_to_highest_priority");  // protect selection and xfer
  if (Print_Scheduler_Verbose) {
    debug_printer->printf("on %d: about to transfer_to_highest_priority %s: ", my_rank(), why);
    print_process_lists(debug_printer);
    debug_printer->nl();
  }
  Oop newProc = find_and_move_to_end_highest_priority_non_running_process();
  if (newProc == roots.nilObj)
    return;

  if (check_many_assertions)  assert(!newProc.as_object()->is_process_running());

  if (Print_Scheduler) {
    debug_printer->printf("on %d: in transfer_to_highest_priority %s: ", my_rank(), why);
    if (Print_Scheduler_Verbose) print_process_lists(debug_printer);
    debug_printer->printf("  found:  ");
    newProc.print_process_or_nil(debug_printer);
    debug_printer->nl();
  }
  transferTo(newProc, why);
}


void Squeak_Interpreter::resume(Oop aProcess, const char* why) {
  /* I just found a tough bug:
     Resume is called, and it asked all the other cores to do a yield.
     The problem is that if a core is in the midst of a remote prim call back to main,
     its interpreter does not have its running_process set, nor can its state be altered.
     Thus to fix this I either need to safepoint or to add a flag to request the yield.
   -- dmu 10/30/08
   */
  if (Print_Scheduler_Verbose) {
    debug_printer->printf("on %d: resume: ", my_rank());
    aProcess.print_process_or_nil(debug_printer);
    debug_printer->printf(" because %s\n", why);
    if (Print_Scheduler_Verbose) print_process_lists(debug_printer);
  }

  {
    Scheduler_Mutex sm("resume");
    Object_p aProcess_obj = aProcess.as_object();
    assert(aProcess_obj->my_list_of_process() == roots.nilObj);
    aProcess_obj->add_process_to_scheduler_list();
  }
  assert(!Scheduler_Mutex::is_held());

  if (Print_Scheduler_Verbose) {
    debug_printer->printf("on %d: mid-resume:\n", my_rank());
    if (Print_Scheduler_Verbose) print_process_lists(debug_printer);
  }
  set_yield_requested(true);
  if (Print_Scheduler_Verbose) {
    debug_printer->printf("on %d: post-resume:\n", my_rank());
    if (Print_Scheduler_Verbose) print_process_lists(debug_printer);
  }
}


void Squeak_Interpreter::signalExternalSemaphores(const char* why) {
  Safepoint_Ability sa(false);

  set_semaphoresUseBufferA(!semaphoresUseBufferA());

  Object_p xArray = splObj_obj(Special_Indices::ExternalObjectsArray);
  oop_int_t xSize = xArray->stSize();
  if (semaphoresUseBufferA()) {
    // use other buffer during read
    for (int i = 1;  i <= semaphoresToSignalCountB();  ++i) {
      int index = _semaphoresToSignalB[i];
      if (1 <= index  &&  index <= xSize) {
        Oop sema = xArray->fetchPointer(index - 1);
        // sema indices 1-based
        if (sema.fetchClass() == splObj(Special_Indices::ClassSemaphore)) {
          sema.as_object()->synchronousSignal(why);
        }
      }
    }
    set_semaphoresToSignalCountB(0);
  }
  else {
    // use other buffer during read
    for (int i = 1;  i <= semaphoresToSignalCountA();  ++i) {
      int index = _semaphoresToSignalA[i];
      if (1 <= index  &&  index <= xSize) {
        Oop sema = xArray->fetchPointer(index - 1);
        // sema indices 1-based
        if (sema.fetchClass() == splObj(Special_Indices::ClassSemaphore))
          sema.as_object()->synchronousSignal(why);
      }
    }
    set_semaphoresToSignalCountA(0);
  }
}


void Squeak_Interpreter::put_running_process_to_sleep(const char* why) {
  Scheduler_Mutex sm("put_running_process_to_sleep"); // am changing a process's state
  Oop aProcess = get_running_process();
  if (aProcess == roots.nilObj) {
    assert_registers_stored();
    return;
  }
  if (Print_Scheduler_Verbose) {
    debug_printer->printf("scheduler: on %d, put_running_process_to_sleep: ", my_rank());
    aProcess.print_process_or_nil(debug_printer);
    debug_printer->printf(", %s\n", why);
  }

  assert(activeContext() != roots.nilObj);
  assert_eq(activeContext_obj(), (void*)activeContext().as_object(), "active context is messed up");
  if (Check_Prefetch)  assert_always(have_executed_currentBytecode);
  storeContextRegisters(activeContext_obj()); // xxxxxx redundant maybe with newActiveContext call in start_running
  aProcess.as_object()->set_suspended_context_of_process(activeContext());
  unset_running_process();
  if (Print_Scheduler_Verbose) {
    debug_printer->printf("scheduler: on %d, AFTER put_running_process_to_sleep: ", my_rank());
    aProcess.print_process_or_nil(debug_printer);
    debug_printer->nl();
    print_process_lists(debug_printer);
  }
  assert_registers_stored();
}


void Squeak_Interpreter::yield(const char* why) {
  if (process_is_scheduled_and_executing())
    assert_external();
  if (Print_Scheduler_Verbose) {
    debug_printer->printf("scheduler on %d: pre yield because %s ", my_rank(), why);
    get_running_process().as_object()->print_process_or_nil(debug_printer);
    debug_printer->nl();
    if (Print_Scheduler_Verbose) print_stack_trace(dittoing_stdout_printer);
  }
  assert(get_running_process() == roots.nilObj  ||  activeContext() != roots.nilObj );
  put_running_process_to_sleep(why);
  assert_registers_stored();
  transfer_to_highest_priority(why);
  if (Check_Prefetch && process_is_scheduled_and_executing()) assert_always(have_executed_currentBytecode);
  if (Print_Scheduler_Verbose) {
    debug_printer->printf("scheduler on %d: post yield because %s ", my_rank(), why);
    get_running_process().as_object()->print_process_or_nil(debug_printer);
    debug_printer->nl();
    print_stack_trace(dittoing_stdout_printer);
  }
}



Oop Squeak_Interpreter::get_running_process() {
  return roots.running_process_or_nil;
}
void Squeak_Interpreter::set_running_process(Oop proc, const char* why) {
  if (Print_Scheduler_Verbose) {
    debug_printer->printf( "scheduler: on %d: set_running_process: ", my_rank());
    proc.print_process_or_nil(debug_printer);
    debug_printer->printf(", %s prim: 0x%x\n", why, Message_Statics::remote_prim_fn);
  }
  assert_stored_if_no_proc();
  assert_stored_if_no_proc();

  roots.running_process_or_nil = proc;
  if (Track_Processes)
    running_process_by_core[my_rank()] = proc;
  schedulerPointer_obj()->storePointer(Object_Indices::ActiveProcessIndex, proc);
  multicore_interrupt_check = true;
}




oop_int_t Squeak_Interpreter::doPrimitiveDiv(Oop rcvr, Oop arg) {
  // rounds neg results towards neg inf, not 0
  oop_int_t ir, ia;
  if (areIntegers(rcvr, arg)) {
    ir = rcvr.integerValue();
    ia =  arg.integerValue();
    success(ia != 0);
  }
  else
    primitiveFail();
  if (!successFlag)
    return 1.0;
  oop_int_t result;
  if (ir > 0) {
    if (ia > 0)
      result = ir / ia;
    else {
      //round neg res -> neg inf
      oop_int_t posArg = -ia;
      result = -( (ir + (posArg - 1)) / posArg);
    }
  }
  else {
    oop_int_t posRcvr = -ir;
    if (ia > 0)
      result = -( (posRcvr + (ia - 1)) / ia );
    else {
      oop_int_t posArg = -ia;
      result = posRcvr / posArg;
    }
  }
  success( Oop::isIntegerValue(result));
  return result;
}

oop_int_t Squeak_Interpreter::doPrimitiveMod(Oop rcvr, Oop arg) {
  oop_int_t ir, ia;
  if (areIntegers(rcvr, arg)) {
    ir = rcvr.integerValue();
    ia =  arg.integerValue();
    success(ia != 0);
  }
  else
    primitiveFail();
  if (!successFlag)
    return 1;
  oop_int_t result = ir % ia;
  // ensure result same sign as arg
  // add in arg if one neg, other pos
  result += ia < 0  ?  (result > 0  ?  ia  :  0)
                    :  (result < 0  ?  ia  :  0);
  success( Oop::isIntegerValue(result));
  return result;
}


int32 Squeak_Interpreter::positive32BitValueOf(Oop x) {
  if (x.is_int()) {
    int v = x.integerValue();
    if (v < 0) {
      primitiveFail();
      return 0;
    }
    return v;
  }
  Object_p xo = x.as_object();
  assertClass(x, splObj(Special_Indices::ClassLargePositiveInteger));
  if (!successFlag)
    return 0;

  int sz = xo->lengthOf();
  if (sz != sizeof(int32)) {
    primitiveFail();
    return 0;
  }
  return (int32)xo->fetchByte(0) + ((int32)xo->fetchByte(1) << 8) + ((int32)xo->fetchByte(2) << 16) + ((int32)xo->fetchByte(3) << 24);
}

int32 Squeak_Interpreter::signed32BitValueOf(Oop x) {
  if (x.is_int()) return x.integerValue();
  Object_p xo = x.as_object();
  Oop largeClass = xo->fetchClass();
  bool neg =
    largeClass == splObj(Special_Indices::ClassLargePositiveInteger)  ?  false :
    largeClass == splObj(Special_Indices::ClassLargeNegativeInteger) ?  true  :
     (primitiveFail(), false);
  if (!successFlag) return 0;
  oop_int_t sz = xo->lengthOf();
  if (sz != sizeof(int32)) { primitiveFail(); return 0; }
  int32 v = (int32)xo->fetchByte(0) + ((int32)xo->fetchByte(1) << 8) + ((int32)xo->fetchByte(2) << 16) + ((int32)xo->fetchByte(3) << 24);
  return neg ? -v : v;
}


int64 Squeak_Interpreter::positive64BitValueOf(Oop x) {
  if (x.is_int()) {
    int v = x.integerValue();
    if (v < 0) {
      primitiveFail();
      return 0;
    }
    return v;
  }
  Object_p xo = x.as_object();
  assertClass(x, splObj(Special_Indices::ClassLargePositiveInteger));
  if (!successFlag)
    return 0;

  u_int32 sz = xo->lengthOf();
  if (sz > sizeof(int64)) {
    primitiveFail();
    return 0;
  }
  int64 v = 0;
  for (u_int32 i = 0;  i < sz;  ++i)
    v |= int64(xo->fetchByte(i)) << (i * 8);
               return v;
}

int64 Squeak_Interpreter::signed64BitValueOf(Oop x) {
  if (x.is_int()) return x.integerValue();
  Object_p xo = x.as_object();
  Oop largeClass = xo->fetchClass();
  bool neg =
    largeClass == splObj(Special_Indices::ClassLargePositiveInteger)  ?  false :
    largeClass == splObj(Special_Indices::ClassLargeNegativeInteger) ?  true  :
      (primitiveFail(), false);
  if (!successFlag) return 0;
  u_oop_int_t sz = xo->lengthOf();
  if (sz > sizeof(int64)) { primitiveFail(); return 0; }
  int64 v = 0;
  for (u_int32 i = 0;  i < sz;  ++i)
    v |= int64(xo->fetchByte(i)) << (i * 8);
  return neg ? -v : v;
}


void Squeak_Interpreter::print_stack_trace(Printer* p, Object_p proc) {
  if (proc == NULL) proc = get_running_process().as_object();
  if (proc == roots.nilObj.as_object()) {
    p->printf("on %d: cannot print stack; no running process\n", my_rank());
    return;
  }
  Oop cntxt = proc->fetchPointer(Object_Indices::SuspendedContextIndex);
  if (cntxt != roots.nilObj) ;
  else if ((cntxt = activeContext()) != roots.nilObj) ;
  else {
    p->printf("on %d: cannot print stack, process 0x%x is running elsewhere\n", my_rank(), (Object*)proc);
    return;
  }
  for (Oop c = cntxt;
       c != roots.nilObj;
       c = c.as_object()->fetchPointer(Object_Indices::SenderIndex)) {
    if (c.bits() == 0) {
      p->printf("context is zero\n");
      break;
    }
    c.as_object()->print_frame(p);
  }
}


uint32_t Squeak_Interpreter::count_stack_depth() {
  Object_p proc = get_running_process().as_object();
  if (proc == roots.nilObj.as_object())
    return 0;

  Oop cntxt = proc->fetchPointer(Object_Indices::SuspendedContextIndex);
  if (cntxt != roots.nilObj) ;
  else if ((cntxt = activeContext()) != roots.nilObj) ;
  else
    return 0;

  
  uint32_t stack_depth = 0;
  for (Oop c = cntxt;
       c != roots.nilObj;
       c = c.as_object()->fetchPointer(Object_Indices::SenderIndex)) {
    if (c.bits() == 0) {
      break;
    }
    stack_depth++;
  }
  
  return stack_depth;
}

void Squeak_Interpreter::print_all_stack_traces(Printer* pr) {
  // print_all_processes_in_scheduler(pr, true);
   print_all_processes_in_scheduler_or_on_a_list(pr, true);
}



void Squeak_Interpreter::print_process_lists(Printer* pr) {
  print_all_processes_in_scheduler(pr, false);
}

void Squeak_Interpreter::print_all_instances_of_class_process(Printer* pr, bool print_stacks) {
  Oop cls = splObj(Special_Indices::ClassProcess);
  for ( Oop instance  = The_Memory_System()->initialInstanceOf(cls);
            instance != roots.nilObj;
            instance  = The_Memory_System()->nextInstanceAfter(instance) ) {
    
    instance.as_object()->print_process_or_nil(pr, print_stacks);
    pr->nl();
  }
}
                                                              

void Squeak_Interpreter::print_all_processes_in_scheduler_or_on_a_list(Printer* pr, bool print_stacks) {
  Oop cls = splObj(Special_Indices::ClassProcess);
  for ( Oop instance  = The_Memory_System()->initialInstanceOf(cls);
       instance != roots.nilObj;
       instance  = The_Memory_System()->nextInstanceAfter(instance) ) {
    if (   instance.as_object()->my_list_of_process() != roots.nilObj
        || is_process_on_a_scheduler_list(instance)) {
      instance.as_object()->print_process_or_nil(pr, print_stacks);
      pr->nl();
    }
  }
}
  
  
bool Squeak_Interpreter::is_process_on_a_scheduler_list(Oop proc) {
  Object_p po = proc.as_object();
  FOR_EACH_READY_PROCESS_LIST(slo, p, processList, this) {
    for ( Object_p ll = processList->fetchPointer(Object_Indices::FirstLinkIndex).as_object();
         ll != roots.nilObj.as_object();
         ll = ll->fetchPointer(Object_Indices::NextLinkIndex).as_object()) {
      if (ll == po)
        return true;
    }
  }
  return false;
}
  
bool Squeak_Interpreter::verify_all_processes_in_scheduler() {
  bool ok = true;
  FOR_EACH_READY_PROCESS_LIST(slo, p, processList, this) {
    for ( Object_p ll = processList->fetchPointer(Object_Indices::FirstLinkIndex).as_object();
         ll != roots.nilObj.as_object();
         ll = ll->fetchPointer(Object_Indices::NextLinkIndex).as_object()) {
      ok = ok && ll->verify_process(); 
    }
  }
  return ok;
}


void Squeak_Interpreter::print_all_processes_in_scheduler(Printer* pr, bool print_stacks) {
  pr->nl();
  FOR_EACH_READY_PROCESS_LIST(slo, p, processList, this) {
    for ( Object_p ll = processList->fetchPointer(Object_Indices::FirstLinkIndex).as_object();
         ll != roots.nilObj.as_object();
         ll = ll->fetchPointer(Object_Indices::NextLinkIndex).as_object()) {
      ll->print_process_or_nil(pr, print_stacks); 
      if (!print_stacks) pr->nl();
    }
  }
  pr->nl();
  if (print_stacks) pr->nl();
}



void Squeak_Interpreter::commonAt(bool stringy) {
  /*
   "This code is called if the receiver responds primitively to at:.
   If this is so, it will be installed in the atCache so that subsequent calls of at:
   or next may be handled immediately in bytecode primitive routines."
   | index rcvr atIx result |
   */
  oop_int_t index = positive32BitValueOf(stackTop());
  Oop rcvr = stackValue(1);
  if (!successFlag || !rcvr.is_mem()) { primitiveFail(); return; }
  Object_p ro = rcvr.as_object();
  /*
   "NOTE:  The at-cache, since it is specific to the non-super response to #at:.
   Therefore we must determine that the message is #at: (not, eg, #basicAt:),
   and that the send is not a super-send, before using the at-cache."
   */
  if (roots.messageSelector == specialSelector(16) &&  roots.lkupClass == ro->fetchClass()) {
    // look in the at cache
    At_Cache::Entry* e = atCache.get_entry(rcvr, false);
    if (!e->matches(rcvr)) {
      e->install(rcvr, stringy);
      Oop result;
      if (successFlag) {
        result = commonVariableAt(rcvr, index, e, false);
         ro = rcvr.as_object(); // recover from GC
      }
      if (successFlag) {
        popThenPush(get_argumentCount() + 1, result);
        return;
      }
    }
  }
  successFlag = true;
  Oop result = stObjectAt(ro, index);
  if (successFlag) {
    if (stringy)
      result = characterForAscii(result.integerValue());
    popThenPush(get_argumentCount() + 1, result);
  }
}


void Squeak_Interpreter::commonAtPut(bool stringy) {
  /*
   "This code is called if the receiver responds primitively to at:Put:.
   If this is so, it will be installed in the atPutCache so that subsequent calls of at:
   or  next may be handled immediately in bytecode primitive routines."
   */
  Oop value = stackTop();
  oop_int_t index = positive32BitValueOf(stackValue(1));
  Oop rcvr = stackValue(2);
  if (!successFlag || !rcvr.is_mem()) {
    primitiveFail();
    return;
  }
  Object_p ro = rcvr.as_object();
  if (roots.messageSelector == specialSelector(17)  &&  roots.lkupClass == ro->fetchClass()) {
    At_Cache::Entry* e = atCache.get_entry(rcvr, true);
    if (!e->matches(rcvr)) {
      e->install(rcvr, stringy);
    }
    if (successFlag)
      commonVariableAtPut(rcvr, index, value, e);
    if (successFlag) {
      popThenPush(get_argumentCount() + 1, value);
      return;
    }
  }
  successFlag = true;
  if (stringy)
    stObjectAtPut(ro, index, asciiOfCharacter(value));
  else
    stObjectAtPut(ro, index, value);
  if (successFlag)
    popThenPush(get_argumentCount() + 1, value);
}




void Squeak_Interpreter::changeClass(Oop rcvr, Oop argClass, bool /* defer */) {
   /*
   "Change the class of the receiver into the class specified by the argument
    given that the format of the receiver matches the format of the argument.
    
    Fail if receiver or argument are SmallIntegers, or the receiver is an
    instance of a compact class and the argument isn't, or when the argument's
    class is compact and the receiver isn't, or when the format of the receiver
    is different from the format of the argument's class, or when the arguments
    class is fixed and the receiver's size differs from the size that an
    instance of the argument's class should have."
   */
  Object_p ro = rcvr.as_object();
  oop_int_t classHdr = argClass.as_object()->formatOfClass();

  // compute size of instances, for fixed field classes
  oop_int_t sizeHiBits = (classHdr & 0x60000) >> 9;
  classHdr &= 0x1ffff;
  int byteSize = (classHdr & (Object::SizeMask | Object::Size4Bit)) | sizeHiBits; // low bits 0

  // check rcvr fmt vs class
  oop_int_t argFormat = (classHdr >> 8) & 0x15;
  oop_int_t rcvrFormat = ro->format();
  if (argFormat != rcvrFormat) { primitiveFail(); return; }

  // for fixed-field classes, sizes must match, byteSize-4 because of baseHeader
  if (Object::Format::has_only_fixed_fields(argFormat)
  &&  byteSize - Object::BaseHeaderSize  !=  ro->byteSize()) {
      primitiveFail();
      return;
    }

  if (ro->headerType() == Header_Type::Short) {
    // compact classes
    oop_int_t ccIndex = classHdr & Object::CompactClassMask;
    if (ccIndex == 0) {
      primitiveFail();  return;
    }
    The_Memory_System()->store_enforcing_coherence(&ro->baseHeader,  (ro->baseHeader & ~Object::CompactClassMask) | ccIndex, ro);
  }
  else {
    // exchange class pointer

    The_Memory_System()->store_enforcing_coherence(&ro->class_and_type_word(),  argClass.bits() | ro->headerType(), ro);
    ro->my_heap()->possibleRootStore(rcvr, argClass);
  }
}


void Squeak_Interpreter::internalExecuteNewMethod() {
  PERF_CNT(this, count_methods_executed());
  
  assert_stored_if_no_proc();
  if (primitiveIndex  > 0) {
    if (255 < primitiveIndex  &&  primitiveIndex < 520) {
      // "Internal return instvars"
      if (264 <= primitiveIndex) {
        internalPopThenPush(1, internalStackTop().as_object()->fetchPointer(primitiveIndex - 264));
        return;
      }
      // "Internal return constants"
      switch (primitiveIndex) {
        case 256: return;
        case 257: internalPopThenPush(1, roots.trueObj); return;
        case 258: internalPopThenPush(1, roots.falseObj); return;
        case 259: internalPopThenPush(1, roots.nilObj); return;
        default:  internalPopThenPush(1, Oop::from_int(primitiveIndex - 261)); return;
      }
    }
    externalizeIPandSP();
    // "self primitiveResponse. <-replaced with  manually inlined code"
    {
      Safepoint_Ability sa(true);
      int nArgs, delta, pi;
      Oop pre_prim_active_context;
      if (DoBalanceChecks) {
        nArgs = get_argumentCount();
        delta = stackPointer() - activeContext_obj()->as_oop_p();
        pi = primitiveIndex; // perform prim can zero out primitiveIndex, compare with pi below (Squeak bug)
        pre_prim_active_context = activeContext();
      }
      successFlag = true;
      assert_stored_if_no_proc();
      dispatchFunctionPointer(primitiveFunctionPointer, do_primitive_on_main); // "branch direct to prim function from address stored in mcache"

      if (DoBalanceChecks  && !balancedStackAfterPrimitive(delta, pi, nArgs, pre_prim_active_context))
        printUnbalancedStack(primitiveIndex, primitiveFunctionPointer);
    }

    if (process_is_scheduled_and_executing())
      internalizeIPandSP();
    
    if (successFlag)
      return;
  }
  // "if not primitive, or primitive failed, activate the method"
  internalActivateNewMethod();
  // "check for possible interrupts at each real send"
  internalQuickCheckForInterrupts();
}


void Squeak_Interpreter::executeNewMethod() {
  /*
   execute a method not found in the mCache - which means
   that primitiveIndex must be manually set.
   
   Used by primitiveValue & primitiveExecuteMethod,
   where no lookup is previously done
   */
  if (primitiveIndex > 0) {
    primitiveResponse();
    if (successFlag)  return;
  }
  activateNewMethod();
  quickCheckForInterrupts();
}


bool Squeak_Interpreter::primitiveResponse() {
  Safepoint_Ability sa(true);
  int nArgs, delta;
  Oop pre_prim_active_context;
  if (DoBalanceChecks)  {
    nArgs = get_argumentCount();
    delta = stackPointer() - activeContext_obj()->as_oop_p();
    pre_prim_active_context = activeContext();
  }
  int primIdx = primitiveIndex;
  successFlag = true;
  dispatchFunctionPointer(primIdx, &primitiveTable);
  if (DoBalanceChecks  &&  !balancedStackAfterPrimitive(delta, primIdx, nArgs, pre_prim_active_context))
    printUnbalancedStack(primIdx, NULL);
  return successFlag;
}


void Squeak_Interpreter::primitivePerformAt(Oop lookupClass) {
  Oop argumentArray = stackTop();
  Object_p aao;
  
  if (!argumentArray.is_mem() || !(aao = argumentArray.as_object())->isArray()) {
    primitiveFail();  return;
  }
  oop_int_t arraySize;
  if (successFlag) {
    arraySize = aao->fetchWordLength();
    oop_int_t cntxSize  = activeContext_obj()->fetchWordLength();
    success(stackPointerIndex() + arraySize  <  cntxSize);
  }
  if (!successFlag)  return;

  Oop performSelector = roots.messageSelector;
  Oop performMethod  = roots.newMethod;
  oop_int_t performArgCount = get_argumentCount();
  popStack();
  roots.messageSelector = popStack();

  // copy args, exec
  for (int index = 1;  index <= arraySize;  ++index)
    push(aao->fetchPointer(index - 1));
  set_argumentCount( arraySize );

  findNewMethodInClass(lookupClass);

  {
  Object_p nmo;
  if (roots.newMethod.is_mem()  && (nmo = newMethod_obj())->isCompiledMethod())
    success(nmo->argumentCount() == get_argumentCount());
  }

  if (successFlag) {
    executeNewMethodFromCache();
    successFlag = true;
  }
  else {
    pop(get_argumentCount());
    push(roots.messageSelector);
    push(argumentArray);
    roots.messageSelector = performSelector;
    roots.newMethod = performMethod;
    set_argumentCount( performArgCount );
  }
}


void Squeak_Interpreter::snapshot(bool /* embedded */) {

  Oop r = popStack();
  pushBool(true);
  
  lprintf("snapshot: quiescing\n");

  // not only quiesces others but gets them to write back activeContext into the processes
  // also puts activeProc to sleep
  
  Oop activeProc = get_running_process();
  {
    Safepoint_for_moving_objects ss("snapshot");
    Safepoint_Ability sa(false);
   
    lprintf("snapshot: quiesced\n");
    {
      Scheduler_Mutex sm("snapshot prep");
      if (!process_is_scheduled_and_executing()) 
        transferTo(activeProc, "snapshot prep");
    }

    {
      Scheduler_Mutex sm("snapshot");
      storeContextRegisters(activeContext_obj());
      remove_running_process_from_scheduler_lists_and_put_it_to_sleep("snapshot");  // unlike Squeak, RVM keeps running procs in list

      schedulerPointer_obj()->storePointer(Object_Indices::ActiveProcessIndex, activeProc);
      assert_active_process_not_nil();
    }
    lprintf("snapshot: starting GC\n");
    The_Memory_System()->fullGC("snapshot");
    lprintf("snapshot: cleaning up\n");
    The_Memory_System()->snapshotCleanUp();
    lprintf("snapshot: writing image\n");
    assert_active_process_not_nil();
    The_Memory_System()->writeImageFile(The_Memory_System()->imageName());
    assert_active_process_not_nil();
    lprintf("snapshot: postGCAction_everywhere\n");
    The_Squeak_Interpreter()->postGCAction_everywhere(false); // With object table, may have moved things

    {
      Scheduler_Mutex sm("snapshot recovery");

      activeProc.as_object()->add_process_to_scheduler_list(); // unlike Squeak, we keep running proc in list
      transferTo(activeProc, "snapshot");
      if (Check_Prefetch) assert_always(have_executed_currentBytecode); // will return from prim and prefetch
    }
  }
  lprintf("snapshot: finishing\n");
  activeContext_obj()->beRootIfOld();
  pop(1);
  if (successFlag)
    pushBool(false);
  else
    push(r);
  
  lprintf("snapshot: returning\n");
}


void Squeak_Interpreter::showDisplayBitsOf(Oop aForm, oop_int_t l, oop_int_t t, oop_int_t r, oop_int_t b) {
  if (deferDisplayUpdates()) return;
  displayBitsOf(aForm, l, t, r, b);
}


void Squeak_Interpreter::displayBitsOf(Oop aForm, oop_int_t l, oop_int_t t, oop_int_t r, oop_int_t b) {
  Oop displayObj = displayObject();
  if (aForm != displayObj)  return;
  Object_p doo;
  Oop dispBits;
  success(displayObj.is_mem()  &&  (doo = displayObj.as_object())->lengthOf() >= 4);
  oop_int_t w, h, d;
  if (successFlag) {
    dispBits = doo->fetchPointer(0);
    w = doo->fetchInteger(1);
    h = doo->fetchInteger(2);
    d = doo->fetchInteger(3);
  }
  oop_int_t left = max(0, l);
  oop_int_t right = min(r, w);
  oop_int_t top = max(0, t);
  oop_int_t bottom = min(b, h);
  if (!successFlag  ||  left > right  ||  top > bottom) {
    return;
  }
  if (dispBits.is_int()) {
    oop_int_t surfaceHandle = dispBits.integerValue();
    if (showSurfaceFn == NULL) {
      showSurfaceFn = The_Interactions.load_function_from_plugin(Logical_Core::main_rank, "ioShowSurface", "SurfacePlugin");
      if (showSurfaceFn == NULL) {
        success(false); return;
      }
    }
    showSurfaceFn(surfaceHandle, left, top, right-left, bottom-top);
  }
  else {
    char* dispBitsIndex = dispBits.as_object()->as_char_p() + Object::BaseHeaderSize;
    assert_on_main();
    ioShowDisplay(dispBitsIndex, w, h, d, left, right, top, bottom);
  }
}

void Squeak_Interpreter::fullDisplayUpdate() {
  Oop displayObj = displayObject();
  if (!displayObj.is_mem()) return;
  Object_p dOo = displayObj.as_object();
  if (!dOo->isPointers()  ||  dOo->lengthOf() < 4)  return;
  displayBitsOf(displayObj, 0, 0, dOo->fetchInteger(1), dOo->fetchInteger(2));
  ioForceDisplayUpdate();
}


// Was: wakeHighestPriority
// xxxxxx factor with remove_process_from_scheduler_list or not -- dmu 4/09
Oop Squeak_Interpreter::find_and_move_to_end_highest_priority_non_running_process() {
  if (!is_ok_to_run_on_me()) {
    return roots.nilObj;
  }
  Scheduler_Mutex sm("find_and_move_to_end_highest_priority_non_running_process");
  // return highest pri ready to run
  // see find_a_process_to_run_and_start_running_it
  bool verbose = false;
  bool found_a_proc = false;
  FOR_EACH_READY_PROCESS_LIST(slo, p, processList, this)  {

    if (processList->isEmptyList())
      continue;
    found_a_proc = true;
    if (verbose)
      lprintf("find_and_move_to_end_highest_priority_non_running_process searching list %d\n", p + 1);
    Oop  first_proc = processList->fetchPointer(Object_Indices::FirstLinkIndex);
    Oop   last_proc = processList->fetchPointer(Object_Indices:: LastLinkIndex);

    Oop        proc = first_proc;
    Object_p proc_obj = proc.as_object();
    Object_p prior_proc_obj = (Object_p)NULL;
    for (;;)  {
      if (verbose) {
        debug_printer->printf("on %d: find_and_move_to_end_highest_priority_non_running_process proc: ",
                              my_rank());
        proc_obj->print_process_or_nil(debug_printer);
        debug_printer->nl();
      }
      OS_Interface::mem_fence(); // xxxxxx Is this fence needed? -- dmu 4/09
      assert(proc_obj->as_oop() == proc  &&  proc.as_object() == proc_obj);
      if (proc_obj->is_process_running()  ||  !proc_obj->is_process_allowed_to_run_on_this_core())
        ;
      else if (last_proc == proc) {
         return proc;
      }
      else if (first_proc == proc) {
        processList->removeFirstLinkOfList();
        processList->addLastLinkToList(proc);
        return proc;
      }
      else {
        processList->removeMiddleLinkOfList(prior_proc_obj, proc_obj);
        processList->addLastLinkToList(proc);
        return proc;
      }
      if  (last_proc == proc)
        break;

      prior_proc_obj = proc_obj;
      proc = proc_obj->fetchPointer(Object_Indices::NextLinkIndex);
      proc_obj = proc.as_object();
    }
  }

  // In a 4.0 image running our prims, there is always at least the idle process in the list
  if (primitiveThisProcess_was_called()  &&  Object::image_is_pre_4_1()) assert_always(found_a_proc);

  OS_Interface::mem_fence(); // xxxxxx Is this fence needed? -- dmu 4/09
  return roots.nilObj;
}


int Squeak_Interpreter::count_processes_in_scheduler() {
  Scheduler_Mutex sm("find_and_move_to_end_highest_priority_non_running_process");
  // return highest pri ready to run
  // see find_a_process_to_run_and_start_running_it
  int count = 0;
  
  FOR_EACH_READY_PROCESS_LIST(slo, p, processList, this)  {
    
    if (processList->isEmptyList())
      continue;
    Oop  first_proc = processList->fetchPointer(Object_Indices::FirstLinkIndex);
    Oop   last_proc = processList->fetchPointer(Object_Indices:: LastLinkIndex);
    
    Oop        proc = first_proc;
    Object_p proc_obj = proc.as_object();
    for (;;)  {
      ++count;
      if  (last_proc == proc)
        break;
      
      proc = proc_obj->fetchPointer(Object_Indices::NextLinkIndex);
      proc_obj = proc.as_object();
    }
  }
  
  return count;
}





void Squeak_Interpreter::copyBits() {
  fn_t f = The_Interactions.load_function_from_plugin(Logical_Core::main_rank, "copyBits", "BitBltPlugin");
  assert(f != NULL);
  if (f == NULL) { primitiveFail(); return; }
  assert_on_main();
  f();
}

void Squeak_Interpreter::copyBitsFromtoat(oop_int_t x0, oop_int_t x1, oop_int_t y) {
  fn_t f = The_Interactions.load_function_from_plugin(Logical_Core::main_rank, "copyBitsFromtoat", "BitBltPlugin");
  if (f == NULL) { primitiveFail(); return; }
  assert_on_main();
  f(x0, x1, y);
}

oop_int_t Squeak_Interpreter::ioFilenamefromStringofLengthresolveAliases(char* aCharBuffer, char* aFilenameString, oop_int_t filenameLength, oop_int_t resolveFlag) {
  /*
   the vm has to convert aFilenameString via any canonicalization and
   char-mapping and put the result in aCharBuffer.
   
   Note the resolveAliases flag - this is an awful artefact of OSX and Apples
   demented alias handling. When opening a file, the flag must be  true,
   when closing or renaming it must be false. Sigh.
   */
  sqGetFilenameFromString(aCharBuffer, aFilenameString, filenameLength, resolveFlag);
  return 0;
}

Oop     Squeak_Interpreter::displayObject() {
  Oop r = splObj(Special_Indices::TheDisplay);
  assert(r.bits() != 0);
  return r;
}


bool Squeak_Interpreter::verify() {
  if (!is_initialized()) return true;
  return
       roots.verify()
    && methodCache.verify()
    && atCache.verify();
}

Oop Squeak_Interpreter::get_stats(int what_to_sample) {
  // output number of transitions, cycles, time in each

  update_times_when_asking();

  static unsigned int last_reported_bytecodes[Memory_Semantics::max_num_threads_on_threads_or_1_on_processes]; // implicit 0 init
  int s = makeArrayStart();
  if (what_to_sample & (1 << SampleValues::bytecodes)) {
    int32 bytecodesExecuted = bcCount - last_reported_bytecodes[rank_on_threads_or_zero_on_processes()];
    PUSH_POSITIVE_32_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(bytecodesExecuted);
    last_reported_bytecodes[rank_on_threads_or_zero_on_processes()] = bcCount;
  }
  if (what_to_sample & (1 << SampleValues::yieldCount)) {
    PUSH_POSITIVE_32_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(yieldCount);
    yieldCount = 0;
  }
  if (what_to_sample & (1 << SampleValues::cycleCounts)) {
    PUSH_POSITIVE_64_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(cyclesRunning);
    cyclesRunning = 0;
    PUSH_POSITIVE_64_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(cyclesWaiting);
    cyclesWaiting = 0;
  }
  if (what_to_sample & (1 << SampleValues::interruptChecks)) {
    PUSH_POSITIVE_32_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(interruptCheckCount);
    interruptCheckCount = 0;
    PUSH_POSITIVE_32_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(unforcedInterruptCheckCount);
    unforcedInterruptCheckCount = 0;
  }
  if (what_to_sample & (1 << SampleValues::movedMutatedObjectStats)) {
    PUSH_POSITIVE_32_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(numberOfMovedMutatedRead_MostlyObjects);
    numberOfMovedMutatedRead_MostlyObjects = 0;
    PUSH_POSITIVE_64_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(cyclesMovingMutatedRead_MostlyObjects);
    cyclesMovingMutatedRead_MostlyObjects = 0;
  }
  if (what_to_sample & (1 << SampleValues::mutexStats)) {
    u_int32 semaphoreMutexAcqCycles = semaphore_mutex.get_and_reset_acq_cycles();
    u_int32 semaphoreMutexRelCycles = semaphore_mutex.get_and_reset_rel_cycles();
    u_int32 schedulerMutexAcqCycles = scheduler_mutex.get_and_reset_acq_cycles();
    u_int32 schedulerMutexRelCycles = scheduler_mutex.get_and_reset_rel_cycles();
    u_int32 safepointAcqCycles = safepoint_mutex.get_and_reset_acq_cycles();
    u_int32 safepointRelCycles = safepoint_mutex.get_and_reset_rel_cycles();

    PUSH_POSITIVE_64_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(semaphoreMutexAcqCycles);
    PUSH_POSITIVE_64_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(semaphoreMutexRelCycles);
    PUSH_POSITIVE_64_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(schedulerMutexAcqCycles);
    PUSH_POSITIVE_64_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(schedulerMutexRelCycles);
    PUSH_POSITIVE_64_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(safepointAcqCycles);
    PUSH_POSITIVE_64_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(safepointRelCycles);
  }
  if (what_to_sample & (1 << SampleValues::interpreterLoopStats)) {
    PUSH_POSITIVE_64_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(perf_counter.get_interpret_cycles());
    PUSH_POSITIVE_64_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(perf_counter.get_multicore_interrupt_cycles());
    PUSH_POSITIVE_64_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(perf_counter.get_mi_cyc_1());
    PUSH_POSITIVE_64_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(perf_counter.get_mi_cyc_1a());
    PUSH_POSITIVE_64_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(perf_counter.get_mi_cyc_1a1());
    PUSH_POSITIVE_64_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(perf_counter.get_mi_cyc_1a2());
    PUSH_POSITIVE_64_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(perf_counter.get_mi_cyc_1b());

    PUSH_POSITIVE_32_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(perf_counter.get_multicore_interrupt_check());
    PUSH_POSITIVE_32_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(perf_counter.get_yield_requested());
    PUSH_POSITIVE_32_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(perf_counter.get_data_available());
    
    perf_counter.reset_accumulators();
  }
  return  makeArray(s);
}



Oop Squeak_Interpreter::makeArray(int start) {
  int n = remapBufferCount - start;

  Object_p r = splObj(Special_Indices::ClassArray).as_object()->instantiateClass(n);

  for (int i = n-1;  i >= 0;  --i)
    r->storePointer(i, popRemappableOop());
  return r->as_oop();
}

bool Squeak_Interpreter::verify_active_context_through_internal_stack_top() {
  Oop* last = localSP();
  for (Oop* p = &activeContext_obj()->as_oop_p()[Object::BaseHeaderSize/sizeof(Oop)];
            p <= last;
          ++p)
    p->verify_oop();
  return true;
}


static char* checkpoint_file_name() {
  static char buf[BUFSIZ];
  snprintf(buf, sizeof(buf) - 1,  "checkpoint_%d", Logical_Core::group_size);
  return buf;
}

static  char end_mark[4] = "end";

void Squeak_Interpreter::save_all_to_checkpoint() {
  FILE* f = fopen(checkpoint_file_name(), "wb");
  if (f == NULL) {
    perror("could not open file for writing");
    fatal("");
  }
  lprintf("checkpointing...\n");

  The_Memory_System()->save_to_checkpoint(f);
  save_to_checkpoint(f);
  // fprintf(stderr, "about to write final mark at 0x%x\n", ftell(f));
  write_mark(f, end_mark);
  // fprintf(stderr, "wrote final mark at 0x%x\n", ftell(f));
  fclose(f);
  lprintf("done checkpointing\n");
  rvm_exit();
}


void Squeak_Interpreter::restore_all_from_checkpoint(int dataSize, int lastHash, int savedWindowSize, int fullScreenFlag) {
  FILE* checkpoint_file = fopen(checkpoint_file_name(), "r");
  if (checkpoint_file == NULL) { perror("cannot open checkpoint file");  OS_Interface::die("could not open checkpoint"); }
  lprintf("restore_all_from_checkpoint...\n");
  The_Memory_System()->restore_from_checkpoint(checkpoint_file, dataSize, lastHash, savedWindowSize, fullScreenFlag);
  restore_from_checkpoint(checkpoint_file);
  // fprintf(stderr, "about to read final mark at 0x%x\n", ftell(checkpoint_file));
  read_mark(checkpoint_file, end_mark);
  // fprintf(stderr, "read final mark at 0x%x\n", ftell(checkpoint_file));
  fclose(checkpoint_file);
  lprintf("restore_all_from_checkpoint done\n");
}

static char check_mark[4] = "sqi";

void Squeak_Interpreter::save_to_checkpoint(FILE* f) {
  write_mark(f, check_mark);

  xfwrite(this, sizeof(*this), 1, f);
}


void Squeak_Interpreter::restore_from_checkpoint(FILE* f) {
  read_mark(f, check_mark);

  u_int64 rm = _run_mask;

  int32 pa = _profile_after, qa = _quit_after;
  bool mc = _make_checkpoint, uc = _use_checkpoint, fe = _fence;

  xfread(this, sizeof(*this), 1, f);
  initialize(roots.specialObjectsOop, true);

  _run_mask = rm;
  _profile_after = pa;  _quit_after = qa;
  _make_checkpoint = mc;  _use_checkpoint = uc;  _fence = fe;
}


// xxxxxx This routine should be reorganized, redone, cleaned up. -- dmu 4/09
// Broke out of the interpreter loop to do some multicore stuff:

void Squeak_Interpreter::multicore_interrupt() {
  if (doing_primitiveClosureValueNoContextSwitch)
    return;
  
  /* Record some performance counters */
  PERF_CNT(this, count_multicore_interrupts());
  if (multicore_interrupt_check)
    PERF_CNT(this, count_multicore_interrupt_check());
  if (yield_requested())
    PERF_CNT(this, count_yield_requested());
  if (Message_Queue::are_data_available(my_core()))
    PERF_CNT(this, count_data_available());

  if (Collect_Performance_Counters)
    const u_int64 start = OS_Interface::get_cycle_count();


  multicore_interrupt_check = false;
  assert_method_is_correct(false, "near start of multicore_interrupt");

  if (process_is_scheduled_and_executing()) {
    internal_undo_prefetch();
    externalizeIPandSP();
  }
  
  {
    Safepoint_Ability sa(true);
    
    move_mutated_read_mostly_objects();
    PERF_CNT(this, add_mi_cyc_1a(OS_Interface::get_cycle_count() - start));

    Message_Statics::process_any_incoming_messages(false);
    PERF_CNT(this, add_mi_cyc_1a1(OS_Interface::get_cycle_count() - start));

    assert_method_is_correct_internalizing(true, "after processing incoming messages");
    PERF_CNT(this, add_mi_cyc_1a2(OS_Interface::get_cycle_count() - start));
    
    safepoint_tracker->spin_if_safepoint_requested();
    PERF_CNT(this, add_mi_cyc_1b(OS_Interface::get_cycle_count() - start));
    
    if (emergency_semaphore_signal_requested) {
      Safepoint_Ability sa(false);

      if (roots.emergency_semaphore.fetchClass() == The_Squeak_Interpreter()->splObj(Special_Indices::ClassSemaphore))
        roots.emergency_semaphore.as_object()->synchronousSignal("emergency signal request");
      emergency_semaphore_signal_requested = false;
    }

    if (yield_requested()) {
      _yield_requested = false;
      yield("yield_requested");
      assert_method_is_correct_internalizing(true, "after fixup_localIP_after_being_transferred_to");
    }

    PERF_CNT(this, add_mi_cyc_1(OS_Interface::get_cycle_count() - start));

    while (!process_is_scheduled_and_executing()) 
      try_to_find_a_process_to_run_and_start_running_it();
  } // end safepoint ability true
  internalizeIPandSP();
  if (Check_Prefetch) assert_always(have_executed_currentBytecode);
  fetchNextBytecode(); // redo prefetch

  PERF_CNT(this, add_multicore_interrupt_cycles(OS_Interface::get_cycle_count() - start));
  
  assert(is_ok_to_run_on_me());
}



void Squeak_Interpreter::try_to_find_a_process_to_run_and_start_running_it() {
  minimize_scheduler_mutex_load_by_spinning_till_there_might_be_a_runnable_process();
  transfer_to_highest_priority("find_a_process_to_run_and_start_running_it");
  assert_method_is_correct_internalizing(true, "after transfer_to_highest_priority");
}


void Squeak_Interpreter::minimize_scheduler_mutex_load_by_spinning_till_there_might_be_a_runnable_process() {
  uint32_t busyWaitCount = 0;
  do {
    safepoint_tracker->spin_if_safepoint_requested(); // since we are about to wait for a message
    Message_Statics::process_any_incoming_messages(false);
    
    if (Logical_Core::running_on_main())  // since we don't run idle process, extra check for events
      ioRelinquishProcessorForMicroseconds(0);
    
    if ( added_process_count < 1  &&  nextPollTick() != 0  &&  idle_cores_relinquish_cpus()) {
      give_up_CPU_instead_of_spinning(busyWaitCount);
    }

    // in case a mouse event came in, and asynchronously signaled a semaphore
    checkForInterrupts(false); // since we don't run idle process, extra check for events
    // Recover from GC if needed
    
  } while (
              added_process_count < 1 /* there are no new procs to run */
              && nextPollTick() != 0     /* forceInterruptCheck was not called */
           ); 
  if (added_process_count) --added_process_count;
}



// STEFAN: think we should try to sleep here and avoid busy waiting too much
// DAVID: the problem is that the sleeps won't wake up if the core receives a request message

void Squeak_Interpreter::give_up_CPU_instead_of_spinning(uint32_t& busyWaitCount) {
  busyWaitCount++;
  
  if (busyWaitCount <= 16)
    return; // NOP
  
  if (busyWaitCount <= 32) {
    if (!Logical_Core::running_on_main())
      OS_Interface::yield_or_spin_a_bit();
    return;
  }
  
  useconds_t sleep = 1 << (busyWaitCount - 32); // wait an exponentially growing time span
  static const u_int32 max_sleep_usecs = 500; // experimentally determined on Mac by watching Kiviats, etc -- dmu 10/1/10
  if (Logical_Core::running_on_main())
    ioRelinquishProcessorForMicroseconds(min(max_sleep_usecs, sleep));
  else 
    usleep(min(max_sleep_usecs, sleep));
}


void Squeak_Interpreter::fixup_localIP_after_being_transferred_to() {
  if (process_is_scheduled_and_executing()) {
    internalizeIPandSP();
    if (Check_Prefetch)  assert_always(have_executed_currentBytecode);
    fetchNextBytecode(); // because normally transferTo is called as primitive, and caller does this
    externalizeIPandSP();
  }
}


void Squeak_Interpreter::move_mutated_read_mostly_objects() {
  if (mutated_read_mostly_objects_count == 0)
    return;

  Safepoint_for_moving_objects sf("move_mutated_read_mostly_objects");
  Safepoint_Ability sa(false);

  if ( print_moves_to_read_write() ) {
    debug_printer->printf("moving %d objects from read-mostly to read-write heap: ",  mutated_read_mostly_objects_count);
  }

  cyclesMovingMutatedRead_MostlyObjects -= OS_Interface::get_cycle_count();
  numberOfMovedMutatedRead_MostlyObjects += mutated_read_mostly_objects_count;

  for (int i = 0;  i < mutated_read_mostly_objects_count;  ++i) {
    if (print_moves_to_read_write() ) {
      mutated_read_mostly_objects[i].as_object()->print(debug_printer);  debug_printer->printf(", ");
    }
    // xxxxxx my_rank() below may not be best--what if heap fills up? -- dmu 4/09
    mutated_read_mostly_objects[i].as_object()->move_to_heap(my_rank(), Memory_System::read_write, true);
    if (mutated_read_mostly_object_tracer() != NULL)
      mutated_read_mostly_object_tracer()->add(mutated_read_mostly_objects[i]);
  }
  if (print_moves_to_read_write() ) debug_printer->nl();
  sync_with_roots(); // don't need full pre/postGCAction_everywhere because we don't move contexts, and caches are oop-based
  mutated_read_mostly_objects_count = 0;
  cyclesMovingMutatedRead_MostlyObjects += OS_Interface::get_cycle_count();
}






void Squeak_Interpreter::run_primitive_on_main_from_elsewhere(fn_t f) {
  dispatchFunctionPointer(f, false);
}


void Squeak_Interpreter::dispatchFunctionPointer(fn_t f, bool on_main) {
  assert_method_is_correct_internalizing(true, "start of dispatchFunctionPointer");
  assert(f);
  Safepoint_Ability sa(true); // prims expect to cope w/ GC -- dmu 5/10
  
# if Dont_Dump_Primitive_Cycles && Dump_Bytecode_Cycles
  static int recurse = 0;
  u_int64 start = OS_Interface::get_cycle_count();
  ++recurse;
# endif

  PERF_CNT(this, count_primitive_invokations());
  
  if (on_main) {
    The_Interactions.run_primitive(Logical_Core::main_rank, f);
    assert_method_is_correct_internalizing(true, "after run_primitive_on_main");
  }
  else {
    (*f)();
    assert_method_is_correct_internalizing(true, "end of dispatchFunctionPointer");
  }
  
# if Dont_Dump_Primitive_Cycles && Dump_Bytecode_Cycles
  --recurse;
  if (recurse == 0) {
    u_int64 dur = OS_Interface::get_cycle_count() - start;
    bc_cycles[bc_cycles_index] += dur - bc_cycles_tare;
  }
# endif
}


void Squeak_Interpreter::booleanCheat(bool cond) {
  // "cheat the interpreter out of the pleasure of handling the next bytecode
  //  IFF it is a jump-on-boolean. Which it is,
  //  often enough when the current bytecode is something like bytecodePrimEqual"
  u_char bytecode = fetchByte(); // assume next bc is jumpIFalse 99%
  internalPop(2);
  if (151 < bytecode  &&  bytecode < 160) {
    // short jumpIfFalse
    if (cond)  fetchNextBytecode();
    else       jump(bytecode - 151);
    return;
  }
  if (bytecode == 172) {
    u_char offset = fetchByte();
    if (cond) fetchNextBytecode();
    else      jump(offset); // always positive
    return;
  }
  // "not followed by a jumpIfFalse; undo instruction fetch and push boolean result"
  set_localIP(localIP() - 1);
  fetchNextBytecode();
  internalPush(cond ? roots.trueObj : roots.falseObj);
}

void Squeak_Interpreter::transferTo(Oop newProc, const char* why) {
  if (check_many_assertions) assert(!newProc.as_object()->is_process_running());

  Scheduler_Mutex sm("transferTo"); // in case another cpu starts running this
  if (Print_Scheduler_Verbose) {
    debug_printer->printf("scheduler: on %d: ", my_rank());
    get_running_process().print_process_or_nil(debug_printer);
    debug_printer->printf( " transferTo ");
    newProc.print_process_or_nil(debug_printer);
    debug_printer->printf(", %s\n", why);
  }
  if (check_many_assertions) assert(!newProc.as_object()->is_process_running());
  put_running_process_to_sleep(why);
  if (check_many_assertions) assert(!newProc.as_object()->is_process_running());
  OS_Interface::mem_fence();
  start_running(newProc, why);
}


void Squeak_Interpreter::start_running(Oop newProc, const char* why) {
  assert_registers_stored();
  assert(Scheduler_Mutex::is_held());
  
  if (newProc == roots.nilObj) {
    // print_process_lists(debug_printer);
    unset_running_process();
    return;
  }
  
  Object_p newProc_obj = newProc.as_object();
  if (newProc_obj->is_process_running()) {
    lprintf("releasing/unset currently running process in start_running\n");
    unset_running_process();
    multicore_interrupt_check = true; // so we stop running
    return;
  }

  set_running_process(newProc, why);
  Oop nac = newProc_obj->get_suspended_context_of_process_and_mark_running();
  Object_p naco = nac.as_object();
  nac.beRootIfOld();
  assert(nac != roots.nilObj); // looking for bug with nil ctx, nonnil proc
  set_activeContext( nac, naco );
  fetchContextRegisters(nac, naco);
  if (Trace_Execution && execution_tracer() != NULL)  execution_tracer()->set_proc(newProc);
  // if (check_many_assertions) assert_always_method_is_correct_internalizing(true, "end of start_running");
  if (Check_Prefetch)  have_executed_currentBytecode = true;
  reclaimableContextCount = 0;
}


void Squeak_Interpreter::newActiveContext(Oop aContext, Object_p aContext_obj) {
  assert(aContext_obj->as_oop() == aContext);
  // internalNewActiveContext must stay consistent with this
  if (process_is_scheduled_and_executing())
    storeContextRegisters(activeContext_obj());
  aContext.beRootIfOld();
  assert(aContext != roots.nilObj); // looking for bug with nil ctx, nonnil proc
  set_activeContext( aContext, aContext_obj );
  fetchContextRegisters(aContext, aContext_obj);
}


void Squeak_Interpreter::commonReturn(Oop localCntx, Oop localVal) {
  Oop nilOop = roots.nilObj;
  assert(localCntx.is_mem());
  Object_p localCntx_obj = localCntx.as_object();

  // make sure can return to given ctx
  if (localCntx == nilOop  ||  localCntx_obj->fetchPointer(Object_Indices::InstructionPointerIndex) == nilOop) {
    // error: sender's IP or ctx is nil
    internalCannotReturn(localVal, localCntx == nilOop, localCntx_obj->fetchPointer(Object_Indices::InstructionPointerIndex) == nilOop, false);
    return;
  }
  // If this return is not to immed predecessor, scan stackfor first unwind marked ctx and inform it.
  for (Oop thisCntx = activeContext_obj()->fetchPointer(Object_Indices::SenderIndex);
       // faster test would be cmp homeContext and activeContext-- see ST
       thisCntx != localCntx;
       thisCntx = thisCntx.as_object()->fetchPointer(Object_Indices::SenderIndex)) {
    if (thisCntx == nilOop) {
      internalCannotReturn(localVal, false, false, true);
      return;
    }
    // climb up; break out of send of aboutToReturn:through: if an unwind marked ctx is found
    bool unwindMarked = thisCntx.as_object()->isUnwindMarked();
    if (unwindMarked) {
      internalAboutToReturn(localVal, thisCntx);
      return;
    }
  }
  // no unwind
  Oop thisCntx = activeContext();
  while (thisCntx != localCntx) {
    Object_p thisCntx_obj = thisCntx.as_object();
    assert(The_Memory_System()->object_table->probably_contains_not(thisCntx_obj));

    Oop contextOfCaller = thisCntx_obj->fetchPointer(Object_Indices::SenderIndex);
    // zap
    thisCntx_obj->zapping_ctx();
    thisCntx_obj->storePointerIntoContext(Object_Indices::SenderIndex, nilOop);
    thisCntx_obj->storePointerIntoContext(Object_Indices::InstructionPointerIndex, nilOop);
    if (reclaimableContextCount > 0) {
      // recycle
      --reclaimableContextCount;
      recycleContextIfPossible_on_its_core(thisCntx);
    }
    thisCntx = contextOfCaller;
  }
  Object_p thisCntx_obj = thisCntx.as_object();
  assert(thisCntx != roots.nilObj);
  set_activeContext( thisCntx, thisCntx_obj);
  thisCntx.beRootIfOld();
  internalFetchContextRegisters(thisCntx, thisCntx_obj);
  fetchNextBytecode();
  internalPush(localVal);
  if (Always_Check_Method_Is_Correct || check_assertions)  check_method_is_correct(false, "end of commonReturn");
}

void Squeak_Interpreter::internalCannotReturn(Oop resultObj, bool b1, bool b2, bool b3) {

  lprintf("internalCannotReturn %d %d %d\n", b1, b2, b3);
  lprintf("this ctx object is 0x%x\n", (Object*)activeContext_obj());
  // fatal("internal cannot return");

  internalPush(activeContext());
  internalPush(resultObj);
  roots.messageSelector = splObj(Special_Indices::SelectorCannotReturn);
  set_argumentCount(1);
  normalSend();
}

void Squeak_Interpreter::internalAboutToReturn(Oop resultObj, Oop aContext) {
  internalPush(activeContext());
  internalPush(resultObj);
  internalPush(aContext);
  roots.messageSelector = splObj(Special_Indices::SelectorAboutToReturn);
  set_argumentCount( 2 );
  normalSend();
}


void Squeak_Interpreter::recycleContextIfPossible_on_its_core(Oop ctx) {
  Object_p ctx_obj = ctx.as_object();
  int rank = ctx_obj->rank();
  if (rank == Logical_Core::my_rank()) 
    recycleContextIfPossible_here(ctx); // optimize critical case
  else {
    // Don't need to preserve oop because it has to be garbage and also because receiver will just recycle it right away.
    recycleContextIfPossibleMessage_class(ctx).handle_here_or_send_to(rank);
  }
}


void Squeak_Interpreter::recycleContextIfPossible_here(Oop ctx) {
  /*
   "If possible, save the given context on a list of free contexts to
   be recycled."
   "Note: The context is not marked free, so it can be reused
   with minimal fuss. The recycled context lists are cleared at
   every garbage collect."
   */
  Object_p ctx_obj = ctx.as_object();
  // unimplemented if (ctx.is_old())  return;

  assert(ctx_obj->rank() == my_rank());

  if (!ctx_obj->isMethodContext())
    return;
  Oop* free_contexts;
  switch (ctx_obj->shortSizeBits()) {
    default: fatal("wrong size"); return;
    case Object_Indices::SmallContextSize:  free_contexts = &roots.freeContexts;       break;
    case Object_Indices::LargeContextSize:  free_contexts = &roots.freeLargeContexts;  break;
  }
  ctx_obj->storePointerIntoContext(Object_Indices::Free_Chain_Index, *free_contexts);
  DEBUG_STORE_CHECK(free_contexts, ctx);
  *free_contexts = ctx;
}




Object_p Squeak_Interpreter::allocateOrRecycleContext(bool needsLarge) {
  if (Trace_GC_For_Debugging  &&  debugging_tracer() != NULL
  &&  debugging_tracer()->force_real_context_allocation())
    ;
  else {
    Oop& freeC = needsLarge ? roots.freeLargeContexts : roots.freeContexts;
    if (freeC != Object::NilContext()) {
      Object_p r = freeC.as_object();
      Oop fc = r->fetchPointer(Object_Indices::Free_Chain_Index);
      assert(fc.is_mem()  ||  fc == Object::NilContext());
      freeC = fc;
      assert(The_Memory_System()->contains(r));
      // assert_eq(r->rank(), my_rank, "");
      if (check_many_assertions  &&  r->get_count_of_blocks_homed_to_this_method_ctx() > 0)
        lprintf("RECYCLING recycled live one 0x%x, method 0x%x\n", r->as_oop().bits(), r->fetchPointer(Object_Indices::MethodIndex).bits());
      return r;
    }
  }

  // xxxxxxxx optimize spl objects by replicating the special objects array someday -- dmu 4/09
  Object_p class_method_context = splObj_obj(Special_Indices::ClassMethodContext);
  const int lcs = Object_Indices::LargeContextSize; // this and next needed for C++ bug
  const int scs = Object_Indices::SmallContextSize;
  Object_p r = class_method_context->instantiateContext(
                                                       needsLarge
                                                       ? lcs
                                                       : scs);

  // "Required init -- above does not fill w/nil.  All others get written."
  r->storePointerIntoContext(Object_Indices::InitialIPIndex, roots.nilObj);
  assert(The_Memory_System()->contains(r));
  assert_eq(r->rank(), my_rank(), "");

  if (check_many_assertions  &&  r->get_count_of_blocks_homed_to_this_method_ctx() > 0)
    lprintf("RECYCLING new live one 0x%x, method 0x%x\n", r->as_oop().bits(), r->fetchPointer(Object_Indices::MethodIndex).bits());

  return r;
}

Oop Squeak_Interpreter::stObjectAt(Object_p a, oop_int_t index) {
  // "Return what ST would return for <obj> at: index."
  oop_int_t fmt = a->format();
  oop_int_t fixedFields = a->fixedFieldsOfArray();
  oop_int_t stSize =  Object::Format::might_be_context(fmt) && a->hasContextHeader()
  ? a->fetchStackPointer()  :  oop_int_t(a->lengthOf() - fixedFields);
  if ( u_oop_int_t(index) >= u_oop_int_t(1)  &&  u_oop_int_t(index) <= u_oop_int_t(stSize))
    return subscript(a, index + fixedFields);
  successFlag = false;
  return roots.nilObj;
}

void Squeak_Interpreter::stObjectAtPut(Object_p a, oop_int_t index, Oop value) {
  oop_int_t fmt = a->format();
  oop_int_t totalLength = a->lengthOf();
  oop_int_t fixedFields = a->fixedFieldsOfArray();
  oop_int_t stSize = Object::Format::might_be_context(fmt) && a->hasContextHeader()
  ? a->fetchStackPointer()  :  totalLength - fixedFields;
  if ( u_oop_int_t(index) >= u_oop_int_t(1)  &&  u_oop_int_t(index) <= u_oop_int_t(stSize))
    subscript(a, index + fixedFields, value);
  else
    successFlag = false;
}



Oop Squeak_Interpreter::subscript(Object_p a, oop_int_t index) {  // rcvr is array
  // "Note: This method assumes that the index is within bounds!"
  oop_int_t fmt = a->format();
  oop_int_t index0 = index - 1; // C is 0-based
  return Object::Format::has_only_oops(fmt)
  ?  a->fetchPointer(index0)
  :  ! Object::Format::has_bytes(fmt)
  ? Object::positive32BitIntegerFor(a->fetchLong32(index0)) // long word objs
  : Oop::from_int(a->fetchByte(index0));
}

void Squeak_Interpreter::subscript(Object_p a, oop_int_t index, Oop value) {
  oop_int_t fmt = a->format();
  if (Object::Format::has_only_oops(fmt))
    a->storePointer(index - 1, value);
  else if (!Object::Format::has_bytes(fmt)) {
    oop_int_t v = positive32BitValueOf(value);
    if (successFlag)
      a->storeLong32(index - 1, v);
  }
  else if (!value.is_int())
    successFlag = false;
  else {
    oop_int_t v = value.integerValue();
    if (v & ~0xff) successFlag = false;
    else a->storeByte(index - 1, v);
  }
}

Logical_Core* Squeak_Interpreter::coreWithSufficientSpaceToInstantiate(Oop klass, oop_int_t indexableSize) {
  /*
   "Return the number of bytes required to allocate an instance of the given 
    class with the given number of indexable fields."
   "Details: For speed, over-estimate space needed for fixed fields or 
    literals; the low space threshold is a blurry line."
   */
  int fmt = (klass.as_object()->formatOfClass() & Object::FormatMask) >> Object::FormatShift;
  if (indexableSize != 0  &&  Object::Format::has_only_fixed_fields(fmt)) // non-indexable
    return NULL;
  int atomSize = !Object::Format::has_bytes(fmt)  ?  sizeof(oop_int_t)  :  1;
  return The_Memory_System()->coreWithSufficientSpaceToAllocate(2500 +  indexableSize * atomSize,
                                                                Memory_System::read_write);
}



Oop Squeak_Interpreter::commonVariableAt(Oop rcvr, oop_int_t index, At_Cache::Entry* e, bool isInternal) {
  oop_int_t stSize = e->size;
  if (1 <= u_int32(index)  &&  u_int32(index) <= u_int32(stSize)) {
    int fmt = e->fmt;
    Object_p rcvr_obj = rcvr.as_object();
    assert_eq(fmt & ~16, rcvr_obj->format(), "format check");

    if (Object::Format::has_only_oops(fmt))
      return rcvr_obj->fetchPointer(index + e->fixedFields - 1);

    if (!Object::Format::has_bytes(fmt)) { // Bitmap
      if (isInternal)
        externalizeIPandSP();

      {
        Safepoint_Ability sa(true);
        return Object::positive32BitIntegerFor(rcvr_obj->fetchLong32(index - 1));
      }
    }

    if (fmt >= 16) // artifical flag for strings
      return characterForAscii(rcvr_obj->fetchByte(index - 1));

    return Oop::from_int(rcvr_obj->fetchByte(index - 1));  // byteArray
  }
  primitiveFail();
  return Oop::from_int(0);
}


void Squeak_Interpreter::commonVariableAtPut(Oop rcvr, oop_int_t index, Oop value, At_Cache::Entry* e) {
  // assumes rcvr has been id'ed at loc atIx in the atCache
  oop_int_t stSize = e->size;
  if (1 <= index  &&  u_int32(index) <= u_int32(stSize) ) {
    int fmt = e->fmt;
    assert_eq(fmt & ~16, rcvr.as_object()->format(), "format check");
    if (Object::Format::has_only_oops(fmt)) {
      int fixedFields = e->fixedFields;
      assert(value.bits());
      rcvr.as_object()->storePointer(index + fixedFields - 1, value);
      return;
    }
    if (!Object::Format::has_bytes(fmt)) { // bitmap
      oop_int_t valToPut = signed32BitValueOf(value); // was positive32BitValueOf
      if (successFlag)
        rcvr.as_object()->storeLong32(index - 1, valToPut);
      return;
    }
    Oop valToPut;
    if (fmt >= 16) { // strings
      valToPut = asciiOfCharacter(value);
      if (!successFlag) return;
    }
    else
      valToPut = value;
    if (valToPut.is_int()) {
      oop_int_t v = valToPut.integerValue();
      if (0 <= v  &&  v <= 255)
        rcvr.as_object()->storeByte(index - 1, v);
      else
        primitiveFail();
      return;
    }
  }
  primitiveFail();
}


void Squeak_Interpreter::trace_execution() {
  if (!Trace_Execution) fatal("cannot trace execution if Trace_Execution is not set");

  Execution_Tracer* tracer;

# if Profile_Image /* necessary, since class will only exists when necessary */
    tracer = new Profiling_Tracer(1000);
# else
    tracer = new Execution_Tracer(1000);
# endif

  set_execution_tracer(tracer);
  lprintf("Tracing execution -- will run slower!!! \n");
}

void Squeak_Interpreter::trace_for_debugging() {
  if (!Trace_GC_For_Debugging) fatal("should never happen");
  set_debugging_tracer(new GC_Debugging_Tracer());
  lprintf("Tracing for debugging\n");
}

void Squeak_Interpreter::print_execution_trace() {
  if (execution_tracer() != NULL) execution_tracer()->print();
}


void Squeak_Interpreter::unset_running_process() {
  set_activeContext(roots.nilObj);
  multicore_interrupt_check = true; // must go into multicore_interrupt to wait for a new process to execute

  assert(    roots.running_process_or_nil == roots.nilObj
         || !roots.running_process_or_nil.as_object()->is_process_running());
  
  roots.running_process_or_nil = roots.nilObj;
  
  if (Track_Processes)
    running_process_by_core[my_rank()] = roots.running_process_or_nil;
  
  assert_registers_stored();
}

bool Squeak_Interpreter::process_is_scheduled_and_executing() {
  assert(activeContext() == roots.nilObj  ||  roots.running_process_or_nil != roots.nilObj);
  return activeContext() != roots.nilObj;
}


// for debugging
void Squeak_Interpreter::check_method_is_correct(bool will_be_fetched, const char* where) {
  const char* msg = "";
  Oop lit;
  int litx = 0;

  if (!process_is_scheduled_and_executing())
    return;




  assert_internal();
  Object_p m = method_obj();
  // m->get_argumentCount()
  // m->temporaryCount()
  u_char* bcp = localIP() + (will_be_fetched ? 1 : 0);
  // only works when regs are stored: u_char* my_ip = activeContext_obj()->next_bc_to_execute_of_context();
  //                                  assert_always(my_ip == bcp);

  m->check_IP_of_method(bcp, activeContext_obj());

  // if (true) return; // always fails before here
  if (Check_Prefetch) assert_always_eq(have_executed_currentBytecode, will_be_fetched);

  if (!will_be_fetched  &&   currentBytecode != *localIP())
    msg = "currentBytecode != *localIP()";
  else if ((litx = literal_index_of_bytecode(bcp)) == -1)
    return;
  else if (!(0 <= litx  &&  litx < m->literalCount()))
    msg = "literal index out of bounds";
  else if (    !(lit = literal(litx)).is_int()
           &&  (Use_Object_Table
                && The_Memory_System()->object_table->probably_contains_not((void*)lit.bits())))
    msg = "bad mem literal";
  else
    return;

  error_printer->printf("on %d: check_method_is_correct: %s, will_be_fetched %d, localIP - method %d, "
                        "bcp - method %d, *bcp %d, litx %d, literalCount %d, lit 0x%x, nbytes %d, at %s\nmethod: ",
                        my_rank(), msg, will_be_fetched,
                        localIP() - m->as_u_char_p(), bcp - m->as_u_char_p(),
                        *bcp, litx, m->literalCount(), lit.bits(), m->lengthOf() - sizeof(Oop),
                        where);
  m->print_compiled_method(error_printer);
  error_printer->nl();

  assert_always_eq(activeContext().bits(), activeContext_obj()->as_oop().bits());
  assert_always_eq(activeContext_obj(), (void*)activeContext().as_object());
  assert_always_eq(method().bits(), activeContext_obj()->fetchPointer(Object_Indices::MethodIndex).bits());
  assert_always_eq(method().as_object(), (void*)method_obj());
  assert_always_eq(method_obj()->as_oop().bits(), method().bits());
  fatal("check_method_is_correct");

}




void Squeak_Interpreter::remember_to_move_mutated_read_mostly_object(Oop x) {
  if (am_receiving_objects_from_snapshot()) return;

  for (int i = 0;  i < mutated_read_mostly_objects_count;  ++i)
    if (mutated_read_mostly_objects[i] == x)
      return;

  multicore_interrupt_check = true;
  if (mutated_read_mostly_objects_count + 1  <  Mutating_Objects_Size)
    mutated_read_mostly_objects[mutated_read_mostly_objects_count++] = x;
  else {
    lprintf("Warning: cannot record mutated_read_mostly object\n");
    breakpoint();
  }
}

void Squeak_Interpreter::receive_initial_interpreter_from_main(Squeak_Interpreter* sq) {
  Safepoint_Ability* const sa = safepoint_ability;
  
  
/* The following hack is needed to get away with the const members in the
   interpreter. The potential optimization should be worth it. */
#if On_Tilera
  *this = *sq;
#else
  const int           my_rank = this->_my_rank;
  Logical_Core* const my_core = this->_my_core;
  
  memcpy(this, sq, sizeof(Squeak_Interpreter)); // initalize with copy; use memcpy to avoid complains about consts, was: *this = *sq;
 
  void* const rankAddr = (void* const)&this->_my_rank;
  void* const coreAddr = (void* const)&this->_my_core;
  *((int*)rankAddr)           = my_rank;
  *((Logical_Core**)coreAddr) = my_core;
#endif
  
  safepoint_tracker = new Safepoint_Tracker();
  safepoint_master_control = NULL;
  safepoint_ability = sa;

  if (check_assertions) {
    assert(roots.specialObjectsOop.is_mem());
    roots.specialObjectsOop.verify_object();
  }

}

void Squeak_Interpreter::print_method_info(const char* msg) {
  error_printer->printf("%d on %d: %s, process_is_scheduled_and_executing %d, method 0x%x, method_obj 0x%x, localIP 0x%x, instructionPointer 0x%x, activeContext_obj() 0x%x, activeContext().as_object() 0x%x, freeContexts 0x%x\n",
                        increment_print_sequence_number(),
                        my_rank(), msg, process_is_scheduled_and_executing(), method().bits(), (Object*)method_obj(), _localIP, _instructionPointer,
                        (Object*)activeContext_obj(), activeContext().as_untracked_object_ptr(),
                        roots.freeContexts.bits());
  if (process_is_scheduled_and_executing()) {
    method_obj()->print_compiled_method(error_printer);
    error_printer->nl();
  }
}


void Squeak_Interpreter::preGCAction_here(bool fullGC) {
  if (check_many_assertions) activeContext_obj()->check_all_IPs_in_chain();
  const bool print = false;
  if (print) print_method_info(fullGC ? "pre preGCAction_here fullGC" : "pre preGCAction_here !fullGC");
  if (process_is_scheduled_and_executing())
    storeContextRegisters(activeContext_obj());
  if (fullGC)   flushInterpreterCaches();
  if (print) print_method_info(fullGC ? "post preGCAction_here fullGC" : "pre preGCAction_here !fullGC");
}


void Squeak_Interpreter::postGCAction_here(bool fullGC) {
  const bool print = false;
  sync_with_roots();
  if (process_is_scheduled_and_executing()) {
    // next line is for assertions only, 
    // because none of the routines called below can receive a message -- dmu 7/12/10
    Safepoint_Ability sa(false); 
    fetchContextRegisters(activeContext(), activeContext_obj());
    internalizeIPandSP(); // may be doing gc deep in msg receiving
    activeContext_obj()->beRootIfOld();
    theHomeContext_obj()->beRootIfOld();


    if (fullGC && Logical_Core::running_on_main()) { // zzzzzz
      signalSema(gcSemaphoreIndex(), "postGCAction_here");
    }
  }
  if (print) print_method_info(fullGC ? "postGCAction_here fullGC" : "postGCAction_here !fullGC");
  if (check_many_assertions) activeContext_obj()->check_all_IPs_in_chain();
  last_gc_bcCount = bcCount;
}


Oop Squeak_Interpreter::remove_running_process_from_scheduler_lists_and_put_it_to_sleep(const char* why) {
  Oop activeProc = get_running_process();
  // Don't need mutex at this level because we remove it first
  activeProc.as_object()->remove_process_from_scheduler_list(why);
  put_running_process_to_sleep(why);
  return activeProc;
}

void Squeak_Interpreter::handle_sigint() {
  lprintf("received sigint and will abort()\n");
  OS_Interface::abort();
}

void Squeak_Interpreter::print_info() {
  print_all_stack_traces(error_printer);
}


void Squeak_Interpreter::signalFinalization(Oop) {
  forceInterruptCheck();
  set_pendingFinalizationSignals(pendingFinalizationSignals() + 1);
}


void Squeak_Interpreter::set_run_mask_and_request_yield(u_int64 x) {
  The_Squeak_Interpreter()->set_run_mask(x);
  The_Squeak_Interpreter()->set_yield_requested(true);
}




void Squeak_Interpreter::broadcast_u_int64(u_int64* d) {
  broadcast_datum(sizeof(*d), d, (u_int64)*d);
}


void Squeak_Interpreter::broadcast_int32(int32* d) {
  broadcast_datum(sizeof(*d), d, (u_int64)*d);
}


void Squeak_Interpreter::broadcast_bool(bool* d) {
  broadcast_datum(sizeof(*d), d, (u_int64)*d);
}


void Squeak_Interpreter::broadcast_datum(int datum_size, void* datum_addr, u_int64 datum) {
  broadcastInterpreterDatumMessage_class m(datum_size, (char*)datum_addr - (char*)this, datum);
  m.send_to_other_cores();
}



void Squeak_Interpreter::distribute_initial_interpreter() {
  Safepoint_Ability sa(false);
  assert_always(Memory_Semantics::cores_are_initialized());
  assert_always(Logical_Core::running_on_main());
  // lprintf("main about to distribute interpreter\n");
  if (check_assertions)  The_Squeak_Interpreter()->roots.specialObjectsOop.verify_object();
  
  // Use a shared buffer to reduce the size of the message to optimize the footprint of message buffer allocation -- dmu & sm
  Squeak_Interpreter* interp_shared_copy = (Squeak_Interpreter*)Memory_Semantics::shared_malloc(sizeof(Squeak_Interpreter));  
  memcpy(interp_shared_copy, The_Squeak_Interpreter(), sizeof(Squeak_Interpreter));
  
  distributeInitialInterpreterMessage_class m(interp_shared_copy);
  m.send_to_other_cores();
  
  free(interp_shared_copy);
}


void Squeak_Interpreter::receive_initial_interpreter() {
  Safepoint_Ability sa(false);
  assert_always(!Logical_Core::running_on_main());
  // lprintf("about to wait for interpreter\n");
  WAIT_FOR_MESSAGE(distributeInitialInterpreterMessage, Logical_Core::main_rank);
  // printf("received interpreter\n");  
}



void Squeak_Interpreter::preGCAction_everywhere(bool fullGC) {
  preGCActionMessage_class(fullGC).send_to_all_cores();
}

void Squeak_Interpreter::postGCAction_everywhere(bool fullGC) {
  // STEFAN: this looks like a good place to invalidate our tracked_ptr's
# if Track_OnStackPointer
  tracked_ptr<Object>::invalidate_all_pointer();
# endif
  
  postGCActionMessage_class(fullGC, safepoint_ability->is_able()).send_to_all_cores();
}

void Squeak_Interpreter::signal_emergency_semaphore() {  
  emergency_semaphore_signal_requested = true;
  set_yield_requested(true);
  multicore_interrupt_check = true;
}

bool Squeak_Interpreter::roomToPushNArgs(int n) {
  /*
   "Answer if there is room to push n arguments onto the current stack.
    There may be room in this stackPage but there may not be room if
    the frame were converted into a context."
   */
  // compiler bug:
  int lcs = Object_Indices::LargeContextSize;
  int scs = Object_Indices::SmallContextSize;
  
  int cntxSize = (( method_obj()->methodHeader() & Object::LargeContextBit ) ? lcs : scs) / bytesPerWord  -  Object_Indices::ReceiverIndex;
  return stackPointerIndex() + n  <=  cntxSize;
}

bool Squeak_Interpreter::getNextEvent_any_platform(void* p) {
  bool preserved_successFlag = successFlag;
  ioGetNextEvent(p);
  int successFlag_value = successFlag;
  successFlag = preserved_successFlag;
  return successFlag_value;
}
