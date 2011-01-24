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


class Squeak_Interpreter {
public:
  Squeak_Interpreter();
  
#if On_Tilera
public:
  inline int my_rank()       const { return Logical_Core::my_rank(); }
  inline Logical_Core* my_core() const { return Logical_Core::my_core(); }
  inline int rank_on_threads_or_zero_on_processes()  const { return Memory_Semantics::rank_on_threads_or_zero_on_processes(); }
  
#else
private:
  // STEFAN: remember thread local information locally
  // should be a lot faster, and we have a Interpreter==Core mapping anyway
  const int _my_rank;
  Logical_Core* const _my_core;
  
public:
  inline int my_rank()       const { return _my_rank; }
  inline Logical_Core* my_core() const { return _my_core; }
  inline int rank_on_threads_or_zero_on_processes()  const { return _my_rank; }
#endif
  void init_rank();
  



  static const int SemaphoresToSignalSize = 500;
	static const int PrimitiveExternalCallIndex = 117; // "Primitive index for #primitiveExternalCall"
	static const int MillisecondClockMask = 0x1FFFFFFF;
	// "Note: The external primitive table should actually be dynamically sized but for the sake of inferior platforms (e.g., Mac :-) who cannot allocate memory in any reasonable way, we keep it static (and cross our fingers...)"
	static const int MaxExternalPrimitiveTableSize = 4096; // "entries"

  public:
  Roots roots;
  Method_Cache methodCache;
  At_Cache atCache;
 private:
  friend class Interpreter_Subset_For_Control_Transfer;
  // these get send for control transfer
  Object_p _activeContext_obj;  //STEFAN TODO come back here and think about the exact meaning and whether we want Object* here
  Object_p _method_obj;
  Object_p _theHomeContext_obj;
 public:
  u_char* _instructionPointer; u_char* instructionPointer() { assert_external(); return _instructionPointer; }  void set_instructionPointer(u_char* x) { registers_unstored(); uninternalized(); _instructionPointer = x; }
  Oop*    _stackPointer;  Oop* stackPointer() { assert_external(); return _stackPointer; } void set_stackPointer(Oop* x) { registers_unstored(); uninternalized(); _stackPointer = x; }
  u_char currentBytecode; // interp version is out of order
  bool   have_executed_currentBytecode;
  oop_int_t interruptCheckCounter;
  static const int interruptCheckCounter_force_value = -0x8000000; // must be neg
  bool multicore_interrupt_check;
  bool doing_primitiveClosureValueNoContextSwitch;
  static const u_int64 all_cores_mask = ~0LL;
  static u_int64 run_mask_value_for_core(int x) { return 1LL << x; }
  void set_run_mask_and_request_yield(u_int64);

  oop_int_t reclaimableContextCount;
  bool successFlag;


# define FOR_ALL_BROADCAST(template) \
  template(int32,int32,semaphoresToSignalCountA, 0) \
  template(int32,int32,semaphoresToSignalCountB, 0) \
  \
  template(u_int64,u_int64,run_mask, all_cores_mask) \
  template(int32,int32,profile_after, -1) \
  template(int32,int32,quit_after, -1) \
  template(bool,bool,make_checkpoint, false) \
  template(bool,bool,use_checkpoint, false) \
  \
  template(bool,bool,fence, true) \
  template(bool,bool,print_moves_to_read_write, false) \
  template(Core_Tracer*,int32,core_tracer, NULL) \
  template(Oop_Tracer*,int32,mutated_read_mostly_object_tracer, NULL) \
  template(Execution_Tracer*,int32,execution_tracer, NULL) \
  template(Debugging_Tracer*,int32,debugging_tracer, NULL) \
  \
  template(bool,bool,am_receiving_objects_from_snapshot, true) \
  \
  template(bool,bool,yield_requested, false) \
  template(int32,int32,process_object_layout_timestamp, 1) \
  template(int32,int32,num_chips, 1)



# define FOR_ALL_FORMERLY_BROADCAST(template) /* just private now */ \
  template(int,int,interruptCheckCounterFeedBackReset,1000)


# define FOR_ALL_HELD_IN_SHARED_MEMORY(template) \
  template(int32,int32,gcSemaphoreIndex, 0) \
  template(int32,int32,interruptKeycode, 2094) /*cmd-.*/ \
  \
  template(int32,int32,nextWakeupTick, 0) \
  template(int32,int32,nextPollTick, 0) \
  template(int32,int32,lastTick, 0) \
  template(int32,int32,pendingFinalizationSignals, 0) \
  template(bool,bool,signalLowSpace, false) \
  template(bool,bool,deferDisplayUpdates, false) \
  template(bool,bool,interruptPending, false) \
  template(bool,bool,idle_cores_relinquish_cpus, false) \
  \
  template(bool,bool,semaphoresUseBufferA, true) \
  template(bool,bool,primitiveThisProcess_was_called, false) \
  \
  template(External_Primitive_Table*, void*,externalPrimitiveTable, new External_Primitive_Table()) \
  \
  template(int, int,dnu_kvetch_count, 0)
  

# define DECL(REAL_T,BROADCAST_T,name,x) REAL_T _ ## name;
  private:
  FOR_ALL_BROADCAST(DECL)
  FOR_ALL_FORMERLY_BROADCAST(DECL)


  struct Shared_memory_fields {
    FOR_ALL_HELD_IN_SHARED_MEMORY(DECL)
# undef DECL
  } *shared_memory_fields;

  int32 _semaphoresToSignalA[SemaphoresToSignalSize]; // just the changed word
  int32 _semaphoresToSignalB[SemaphoresToSignalSize]; // just the changed word


# define GET_AND_SET(REAL_T,BROADCAST_T,name,init_val) \
  REAL_T name() { return _ ## name; }  \
  void set_ ## name(REAL_T x) { \
    _ ## name = x; \
    broadcast_ ## BROADCAST_T((BROADCAST_T*)&_ ## name); \
  }
public:
  FOR_ALL_BROADCAST(GET_AND_SET)
# undef GET_AND_SET

# define GET_AND_SET(REAL_T,BROADCAST_T,name,init_val) \
  REAL_T name() { return _ ## name; } \
  void set_ ## name(REAL_T x) { _ ## name = x; }

  FOR_ALL_FORMERLY_BROADCAST(GET_AND_SET)
# undef GET_AND_SET

# define GET_AND_SET(REAL_T,BROADCAST_T,name,init_val) \
  REAL_T name() { return shared_memory_fields->_ ## name; } \
  void   set_ ## name(REAL_T x) { shared_memory_fields->_ ## name = x; }

  FOR_ALL_HELD_IN_SHARED_MEMORY(GET_AND_SET)
# undef GET_AND_SET

  int32 semaphoresToSignalA(int i) { return _semaphoresToSignalA[i]; }
  void set_semaphoresToSignalA(int i, int x)  {
    _semaphoresToSignalA[i] = x;
    broadcast_int32(&_semaphoresToSignalA[i]);
  }


 private:
  static const int RemapBufferSize = 1024;
  Oop remapBuffer[RemapBufferSize];
  int remapBufferCount;

  static const int Mutating_Objects_Size = 1024;
  Oop mutated_read_mostly_objects[Mutating_Objects_Size];
  int mutated_read_mostly_objects_count;

  bool I_am_running;


 public:
  u_int64 interpret_cycles, multicore_interrupt_cycles, mi_cyc_1, mi_cyc_1a, mi_cyc_1a1, mi_cyc_1a2, mi_cyc_1b;
  int multicore_interrupt_check_count, yield_request_count, data_available_count;

  void remember_to_move_mutated_read_mostly_object(Oop x);

 public:
  Safepoint_Tracker* safepoint_tracker;
  Safepoint_Master_Control* safepoint_master_control;

  u_char* _localIP;  u_char* localIP() { assert_internal(); return _localIP; }  void set_localIP(u_char* x) { _localIP = x; registers_unstored(); unexternalized(); }
  Oop*    _localSP;  Oop*    localSP() { assert_internal(); return _localSP; }  void set_localSP(Oop* x)    { _localSP = x; registers_unstored(); unexternalized(); }
  Object_p _localHomeContext;  Object_p localHomeContext() { assert_internal(); return _localHomeContext; } void set_localHomeContext(Object_p x) { _localHomeContext = x; registers_unstored(); unexternalized(); }
  
  int32 image_version;

# if check_assertions
  bool are_registers_stored;
  bool is_external_valid;
  bool is_internal_valid;
  void registers_unstored() { assert(get_running_process() != roots.nilObj); are_registers_stored = false; }
  void registers_stored() { are_registers_stored = true; }
  void externalized() { is_external_valid = true; }
  void internalized() { is_internal_valid = true; }
  void unexternalized() { is_external_valid = false; }
  void uninternalized() { is_internal_valid = false; }

  void assert_internal() { assert(is_internal_valid); }
  void assert_external() { assert(is_external_valid); }
  void assert_registers_stored() { assert(are_registers_stored); }
  void assert_registers_unstored() { assert(!are_registers_stored); }
  void assert_stored_if_no_proc() { if (check_many_assertions) assert(get_running_process() != roots.nilObj || are_registers_stored); }
# else
  void registers_unstored() {}
  void registers_stored() {}
  void externalized() { }
  void internalized() { }
  void unexternalized() {}
  void uninternalized() { }

  void assert_internal() {  }
  void assert_external() { }
  void assert_registers_stored() { }
  void assert_registers_unstored() {  }
  void assert_stored_if_no_proc() { }
# endif



  oop_int_t primitiveIndex;



  u_char  prevBytecode; // interp version is out of order
  int     _argumentCount;
  int     get_argumentCount() { return _argumentCount; }
  void    set_argumentCount(int x) { _argumentCount = x; } // for debugging, easy point to catch changes -- dmu 5/10

  fn_t    primitiveFunctionPointer; bool do_primitive_on_main;



  oop_int_t interruptChecksEveryNms;



  fn_t    showSurfaceFn;
  int     globalSessionID;

  oop_int_t bcCount, last_gc_bcCount;

  int yieldCount;
  int interruptCheckCount, unforcedInterruptCheckCount;
  u_int64 cyclesRunning, cyclesWaiting, cycles_at_yield, cycles_at_resume;
  u_int64 cyclesMovingMutatedRead_MostlyObjects;
  u_int32 numberOfMovedMutatedRead_MostlyObjects;

  int     methodArgumentCount() { return get_argumentCount(); }
  int     methodPrimitiveIndex() { return primitiveIndex; }



 private:
  Object_p get_addr_to_cache(Oop x) { return x.as_object_if_mem(); }

 public:


# define DO_ALL_CACHED_OBJS(template) \
  template(roots._activeContext,_activeContext_obj) \
  template(roots._method,_method_obj) \
  template(roots._theHomeContext,_theHomeContext_obj)



  void sync_with_root(Oop x) {
# define SWR(oopname,objname) if (oopname == x) objname = get_addr_to_cache(oopname);
    DO_ALL_CACHED_OBJS(SWR)
# undef SWR
    assert_eq(activeContext_obj(), (void*)activeContext().as_object(), "activeContext messed up");
  }

  void sync_with_roots() {
# define SWRS(oopname,objname) objname = get_addr_to_cache(oopname);
    DO_ALL_CACHED_OBJS(SWRS)
# undef SWRS
    assert_eq(activeContext_obj(), (void*)activeContext().as_object(), "activeContext messed up");
  }


  bool need_to_sync_with_roots() { // for debugging
# define NTSWRS(oopname,objname) if (objname != get_addr_to_cache(oopname)) return true;
    DO_ALL_CACHED_OBJS(NTSWRS)
# undef NTSWRS
    return false;
  }


  void pushRemappableOop(Oop x) { remapBuffer[remapBufferCount++] = x; }
  Oop  popRemappableOop()       {return remapBuffer[--remapBufferCount]; }
  void popRemappableOops(int n) { remapBufferCount -= n; assert(remapBufferCount >= 0); }


  void do_all_roots(Oop_Closure* oc);


 public:
   int added_process_count;

   OS_Mutex_Interface* get_scheduler_mutex() {  return &scheduler_mutex; }
   OS_Mutex_Interface* get_semaphore_mutex() {  return &semaphore_mutex; }
   OS_Mutex_Interface* get_safepoint_mutex() {  return &safepoint_mutex; }

  private:
   OS_Mutex_Interface scheduler_mutex;
   OS_Mutex_Interface semaphore_mutex;
   OS_Mutex_Interface safepoint_mutex; // this is not a complete mutex, it does not use underlying systems mutex

   int* global_sequence_number;
   int* print_sequence_number;
   bool* debug_flag;

  public:
   int* debug_int;

   int get_global_sequence_number() { return *global_sequence_number; }
   int increment_global_sequence_number() { return ++*global_sequence_number; }
   int get_print_sequence_number() { return *print_sequence_number; }
   int increment_print_sequence_number() { return ++*print_sequence_number; }

  bool get_debug_flag() { return *debug_flag; }
   void set_debug_flag(bool b) { *debug_flag = b; }

  Oop* running_process_by_core; // array per core of which process that core is running

  int32* timeout_deferral_counters; // for deferring timeouts during long ops
  
  bool emergency_semaphore_signal_requested;


 public:

  Oop activeContext() { return roots._activeContext; }
  Object_p activeContext_obj() { return _activeContext_obj; }

  void set_activeContext(Oop x, Object_p o) {
    assert_eq(o->as_oop().bits(), x.bits(), "activeContext messed up");
    roots._activeContext = x;
    _activeContext_obj = o;
    uninternalized();
    unexternalized();
  }
  void set_activeContext(Oop x) { set_activeContext(x, x.as_object()); }
  void set_activeContext(Object_p x) { set_activeContext( x->as_oop(), x); }


  Oop method() { return roots._method; }
  Object_p method_obj() { return _method_obj; }
  void set_method(Oop m) { roots._method = m;  _method_obj = m.as_object(); }
  void set_method_obj(Object_p m) { roots._method = m->as_oop();  _method_obj = m; }

  Oop theHomeContext() { assert_external(); return roots._theHomeContext; }
  Object_p theHomeContext_obj() { assert_external(); return _theHomeContext_obj; }
  void set_theHomeContext(Oop m, bool really_changing) { if (really_changing) {registers_unstored(); uninternalized(); }  roots._theHomeContext = m;  _theHomeContext_obj = m.as_object(); }
  void set_theHomeContext_obj(Object_p m, bool really_changing) { if (really_changing) {registers_unstored(); uninternalized(); }   roots._theHomeContext = m->as_oop();  _theHomeContext_obj = m; }


  Object_p receiver_obj() { return roots.receiver.as_object(); }
  Object_p newMethod_obj() { return roots.newMethod.as_object(); }
  Object_p lkupClass_obj();

  void print_method_info(const char* msg);
  void preGCAction_here(bool fullGC);
  void postGCAction_here(bool fullGC);
  void preGCAction_everywhere(bool fullGC);
  void postGCAction_everywhere(bool fullGC);



  void initialize(Oop, bool from_checkpoint);
  bool is_initialized();

  void receive_initial_interpreter_from_main(Squeak_Interpreter*);

  void flushInterpreterCaches();
  void flushObsoleteIndexedPrimitives();
  void flushExternalPrimitiveTable();

  void primitiveFail() { successFlag = false; }
  bool failed() { return !successFlag; }
  void success(bool b) { successFlag = successFlag && b; }


  void loadInitialContext();
  void initialCleanup();

  void fetchContextRegisters(Oop cntx, Object_p cntx_obj) {
    assert(cntx_obj->as_oop() == cntx);
    // "if the MethodIndex field is an integer, activeCntx is a block context"
    // "otherwise, it is a method context and is its own home context "
    set_theHomeContext(
                       cntx_obj->is_this_context_a_block_context()
                       ? cntx_obj->fetchPointer(Object_Indices::HomeIndex).beRootIfOld()
                       : cntx,
                       true);
    externalized();
    uninternalized();

    roots.receiver = theHomeContext_obj()->fetchPointer(Object_Indices::ReceiverIndex);
    set_method(theHomeContext_obj()->fetchPointer(Object_Indices::MethodIndex));

    /*
     "the instruction pointer is a pointer variable equal to
     method oop + ip + BaseHeaderSize
     -1 for 0-based addressing of fetchByte
     -1 because it gets incremented BEFORE fetching currentByte "
     */
    oop_int_t ip_int = cntx_obj->quickFetchInteger(Object_Indices::InstructionPointerIndex);
    _instructionPointer = method_obj()->as_u_char_p() + ip_int + Object::BaseHeaderSize - 2;

    // "the stack pointer is a pointer variable also..."
    oop_int_t sp_int = cntx_obj->quickFetchInteger(Object_Indices::StackPointerIndex);
    _stackPointer = (Oop*) (cntx_obj->as_char_p() + Object::BaseHeaderSize + (Object_Indices::TempFrameStart + sp_int - 1) * bytesPerWord);


    if (PrintFetchedContextRegisters) {
      dittoing_stdout_printer->printf("fetchContextRegisters: theHomeContext(): ");
      theHomeContext().print(dittoing_stdout_printer);
      dittoing_stdout_printer->printf(", activeContext: ");
      cntx.print(dittoing_stdout_printer);
      dittoing_stdout_printer->printf(", receiver: ");
      roots.receiver.print(dittoing_stdout_printer);
      dittoing_stdout_printer->printf(", method: ");
      method().print(dittoing_stdout_printer);
      dittoing_stdout_printer->printf(", IP %d, SP %d\n", ip_int, sp_int);
      dittoing_stdout_printer->printf("  instructionPointer 0x%x, stackPointer 0x%x\n",
                                      _instructionPointer, _stackPointer);
    }
  }

  void storeContextRegisters(Object_p cntx_obj) {
    /*
     like internalStoreContextRegisters

     "InstructionPointer is a pointer variable equal to
     method oop + ip + BaseHeaderSize
     -1 for 0-based addressing of fetchByte
     -1 because it gets incremented BEFORE fetching currentByte"
     */
    if (cntx_obj == NULL)
      return;
    cntx_obj->storeIntegerUnchecked_into_context(Object_Indices::InstructionPointerIndex,
                                    instructionPointer() - method_obj()->as_u_char_p() - Object::BaseHeaderSize + 2 );
    cntx_obj->storeIntegerUnchecked_into_context(Object_Indices::StackPointerIndex,
                                    stackPointerIndex() - Object_Indices::TempFrameStart + 1);

    registers_stored();
  }

  void flushExternalPrimitives();

  void interpret();
  void browserPluginInitialiseIfNeeded() {}
  void browserPluginReturnIfNeeded() {}

  void internalizeIPandSP() {
    // Copy local instruction ptr and SP to locals for speed
    assert_external();
    assert(safepoint_ability->is_unable());
    _localIP = instructionPointer();
    _localSP =       stackPointer();
    _localHomeContext = theHomeContext_obj();
    internalized();
  }

  void externalizeIPandSP() {
    // copy out for primitives, etc
    assert_internal();
    _instructionPointer = localIP();
    _stackPointer = localSP();
    set_theHomeContext_obj(localHomeContext(), false);
    externalized();
  }

  inline void traceFetchNextBytecode(u_char currentBytecode) {
    extern FILE* BytecodeTraceFile;

    if (CheckByteCodeTrace && BytecodeTraceFile != NULL) {
      static const int n = 10;
      static bool initStatics = true;
      static int bc_index[Memory_Semantics::max_num_threads_on_threads_or_1_on_processes];

      if (initStatics) {
        initStatics = false;
        for (size_t iter = 0; iter < Memory_Semantics::max_num_threads_on_threads_or_1_on_processes; iter++) {
          bc_index[iter] = -1;
        }
      }

      static u_char bcs[Memory_Semantics::max_num_threads_on_threads_or_1_on_processes][n];   // threadsafe
      static int counts[Memory_Semantics::max_num_threads_on_threads_or_1_on_processes][n];   // threadsafe

      int rank_on_threads_or_zero_on_processes = Memory_Semantics::rank_on_threads_or_zero_on_processes();

      u_char c = fgetc(BytecodeTraceFile);
      if (c != currentBytecode) {
        lprintf("mismatch at %d\n", bcCount);
        for (int j = bc_index[rank_on_threads_or_zero_on_processes] +1;  j != bc_index[rank_on_threads_or_zero_on_processes];  ) {
          if (++j >= n) j = 0;
          lprintf("  %d: ", counts[rank_on_threads_or_zero_on_processes][j]);
          printBC(bcs[rank_on_threads_or_zero_on_processes][j], dittoing_stdout_printer);  dittoing_stdout_printer->nl();
        }
        fatal();
      }
      if (++(bc_index[rank_on_threads_or_zero_on_processes]) >= n)  bc_index[rank_on_threads_or_zero_on_processes] = 0;
      bcs[rank_on_threads_or_zero_on_processes][bc_index[rank_on_threads_or_zero_on_processes]] = currentBytecode;
      counts[rank_on_threads_or_zero_on_processes][bc_index[rank_on_threads_or_zero_on_processes]] = bcCount;
    }

    ++bcCount;

    if (MakeByteCodeTrace && BytecodeTraceFile != NULL) {
      fputc(currentBytecode, BytecodeTraceFile);
      static int last = 20000;
      if (bcCount > last) {
        fflush(BytecodeTraceFile);
        last += 20000;
      }
    }
    if (profile_after() >= 0  &&  bcCount > profile_after())
      OS_Interface::profiler_enable();
    if (quit_after() >= 0  &&  bcCount > quit_after())
      primitiveQuit();
  }

  void fetchNextBytecode() {
    /* "This method fetches the next instruction (bytecode).
        Each bytecode method is responsible for fetching the next bytecode,
        preferably as early as possible to allow the memory system time
        to process the request before the next dispatch." */

    
# if Track_Last_BC_For_Debugging
    prev_pc_for_debugging = currentBytecode;
    prev_bc_addr_for_debugging = bc_addr_for_debugging;
    bc_addr_for_debugging = localIP();
# endif
    
    prevBytecode = currentBytecode;
    currentBytecode = fetchByte();
    if (Check_Prefetch)  have_executed_currentBytecode = false;

    if (!Dont_Trace_Bytecode_Fetching)
      traceFetchNextBytecode(currentBytecode);
  }

  u_char fetchByte() {
    if (check_many_assertions) {
      int localIP_offset = localIP() - method_obj()->as_u_char_p() + 1;
      assert( localIP_offset
             >= (Object_Indices::LiteralStart + Object::literalCountOfHeader(method_obj()->methodHeader())) * bytesPerWord  +  1);
      assert(localIP_offset  <   method_obj()->byteLength() + Object::BaseHeaderSize);
    }

    set_localIP(localIP() + 1);
    return *localIP();
  }

  void dispatch(u_char currentByte);
  void printBC(u_char, Printer*);
  static const char* bytecode_name(u_char bc);



  void pushReceiverVariable(int i) {
    internalPush(receiver_obj()->fetchPointer(i));
  }
  void pushTemporaryVariable(int temporaryIndex) {
	  internalPush(temporary(temporaryIndex));
  }
  void pushLiteralConstant(int i) { internalPush(literal(i)); }
  void pushLiteralVariable(int i) {
    internalPush(literal(i).as_object()->fetchPointer(Object_Indices::ValueIndex));
  }

  Oop temporary(int offset) {
    assert(localHomeContext() != roots.nilObj.as_object());
    return localHomeContext()->fetchPointer(offset + Object_Indices::TempFrameStart);
  }

  void internalPush(Oop x) {
    if (check_many_assertions) {
      Oop* p = (Oop*)activeContext_obj()->nextChunk();
      assert(p == NULL  ||  localSP() + 1  <  p);
      x.verify_oop();
      assert(!The_Memory_System()->heap_containing(localSP())->is_read_mostly());
    }
    set_localSP(localSP() + 1);
    DEBUG_STORE_CHECK(localSP(), x);
    *localSP() = x;
  }
  Oop internalStackTop() { return *localSP(); }
  Oop stackTop() {return *stackPointer(); }
  Oop popStack() { Oop r = *stackPointer(); set_stackPointer(stackPointer() - 1); return r; }

  Oop stackValue(oop_int_t offset) { return stackPointer()[-offset]; }


  double stackFloatValue(oop_int_t offset) {
    Oop floatOop = stackPointer()[-offset];
    if (floatOop.fetchClass() != splObj(Special_Indices::ClassFloat)) {
      primitiveFail();  return 0.0;
    }
    return floatOop.as_object()->fetchFloatAtinto();
  }


  oop_int_t stackIntegerValue(oop_int_t offset) {
    return stackPointer()[-offset].checkedIntegerValue();
  }
  Oop stackObjectValue(int offset) {
    Oop oop = stackPointer()[-offset];
    if (oop.is_mem())  return oop;
    primitiveFail();
    return Oop::from_int(0);
  }
  oop_int_t integerValueOf(Oop x) {
    return x.is_int() ? x.integerValue() : (primitiveFail(), 0);
  }
  bool booleanValueOf(Oop x) {
    return
      x == roots.trueObj ? true :
      x == roots.falseObj ? false :
      (successFlag = false);
  }

  double floatValueOf(Oop x) {
    return x.is_mem() ? floatValueOf(x.as_object())
                      : (success(false), 0.0);
  }
  double floatValueOf(Object_p x) {
    assertClass(x, splObj(Special_Indices::ClassFloat));
    return successFlag ? x->fetchFloatAtinto() : 0.0;
  }


  Oop literal(oop_int_t offset) {
    return method_obj()->literal(offset);
  }
  Oop internalStackValue( int offset )  { return localSP()[-offset]; }
  void internalPop(int n) { set_localSP(localSP() - n); }
  void pop(int n) { set_stackPointer(stackPointer() - n); }
  void unPop(int n) { set_stackPointer(stackPointer() + n); }

  void push(Oop x) { set_stackPointer(stackPointer() + 1); DEBUG_STORE_CHECK(stackPointer(), x);  *stackPointer() = x; }

  void pushFloat(double d) { push(Object::floatObject(d)); }
  void pushBool(bool b) { push(b ? roots.trueObj : roots.falseObj) ; }
  void pushInteger(oop_int_t i) { push(Oop::from_int(i)); }

#if Extra_Preheader_Word_Experiment
  Oop modify_send_for_preheader_word(Oop rcvr);
#endif

  void normalSend() {
    /*
     "Send a message, starting lookup with the receiver's class."
     "Assume: messageSelector and get_argumentCount() have been set, and that
     the receiver and arguments have been pushed onto the stack,"
     "Note: This method is inlined into the interpreter dispatch loop."
     */
    Oop rcvr = internalStackValue(get_argumentCount());

#if Extra_Preheader_Word_Experiment 
    if (rcvr.is_mem() 
        && Oop::from_bits(rcvr.as_object()->get_extra_preheader_word()) != Oop::from_int(0) 
        && roots.messageSelector != roots.extra_preheader_word_selector
        && roots.extra_preheader_word_selector != roots.nilObj
        && !roots.messageSelector.as_object()->starts_with_string("perform") // an escape!
        ) 
      rcvr = modify_send_for_preheader_word(rcvr);
#endif

    roots.receiverClass = roots.lkupClass = rcvr.fetchClass();

    assert(  roots.lkupClass.verify_oop()
           &&     roots.lkupClass.is_mem()
           &&     roots.lkupClass.as_object()->my_heap_contains_me()
           &&     roots.lkupClass != roots.nilObj);

    commonSend();
  }



  void superclassSend() {
    /*
     "Send a message to self, starting lookup with the superclass of the class
      containing the currently executing method."
     "Assume: messageSelector and get_argumentCount() have been set, and that 
      the receiver and arguments have been pushed onto the stack,"
     */
    roots.lkupClass = method_obj()->methodClass().as_object()->superclass();
    assert(roots.lkupClass.verify_oop());
    roots.receiverClass = internalStackValue(get_argumentCount()).fetchClass();
    commonSend();
  }

  inline void debugCommonSend() {
    /* assertions and a breakpoint to aid debugging */

    static int nMatches = 1;    // is not threadsafe, but logic seems to be flawed anyway, nMatches is not reset, so its not the NthSendForStopping, but only after overflow??

    if (PrintSends) {
      Printer* p = stdout_printer;
      p->printf("commonSend Rcvr: ");
      internalStackValue(get_argumentCount()).print(p);
      p->printf(" Class: ");
      roots.receiverClass.print(p);
      p->printf(" Sel: ");
      roots.messageSelector.print(p);
      for (int i = get_argumentCount() - 1;  i >= 0;  --i)
        p->printf("  "), internalStackValue(i).print(p);
      p->nl();

    }
    if (StopOnSend && roots.messageSelector.as_object()->equals_string(StopOnSend)
        &&  (NthSendForStopping <= 0 || nMatches++ == NthSendForStopping))
      breakpoint();
    if (check_assertions && roots.messageSelector.as_object()->equals_string("error:")) { // xxx_dmu
      stdout_printer->printf(" error: "); internalStackTop().print(stdout_printer); stdout_printer->nl();
      roots.messageSelector.print(dittoing_stdout_printer), stdout_printer->nl();
    }
    if (check_assertions && roots.messageSelector.as_object()->equals_string("openNotifierContents:label:")) // xxx_dmu
      dittoing_stdout_printer->printf("on %d: ", my_rank()), roots.messageSelector.print(dittoing_stdout_printer), dittoing_stdout_printer->nl();

    if (check_assertions && roots.messageSelector.as_object()->equals_string("primitiveFailed")) // xxx_dmu
      roots.messageSelector.print(dittoing_stdout_printer), 
      dittoing_stdout_printer->nl();

    if (check_assertions && roots.messageSelector.as_object()->equals_string("yourSelectorHere")) // xxx_dmu
      roots.messageSelector.print(dittoing_stdout_printer),
      dittoing_stdout_printer->nl(),
      print_all_stack_traces(dittoing_stdout_printer);

    if (check_assertions && roots.messageSelector.as_object()->equals_string("cannotReturn:")) // xxx_dmu
      print_stack_trace(dittoing_stdout_printer), breakpoint();
  }


  void commonSend() {
    /*
     "Send a message, starting lookup with the receiver's class."
     "Assume: messageSelector and get_argumentCount() have been set, and that
     the receiver and arguments have been pushed onto the stack,"
     "Note: This method is inlined into the interpreter dispatch loop."
     */

    debugCommonSend();

    internalFindNewMethod();
    internalExecuteNewMethod();
    if (do_I_hold_baton()) // xxxxxxx predicate only needed to satisfy assertions?
      fetchNextBytecode();
  }


  void internalFindNewMethod() {
    /*
     "Find the compiled method to be run when the current messageSelector is
      sent to the class 'roots.lkupClass', setting the values of
      'roots.newMethod' and 'primitiveIndex'."
     */
    if (!lookupInMethodCacheSel(roots.messageSelector, roots.lkupClass)) {
      // "entry was not found in the cache; look it up the hard way"
      externalizeIPandSP();
      {
        Safepoint_Ability sa(true);
        lookupMethodInClass(roots.lkupClass); // may have to allocate message obj
      }
      internalizeIPandSP();
      addNewMethodToCache();
    }
  }


  void internalExecuteNewMethod();

  void internalActivateNewMethod();


  void activateNewMethod();
# if Include_Closure_Support
  void activateNewClosureMethod(Object_p, Object_p);
# endif


  void internalNewActiveContext(Oop aContext, Object_p aContext_obj) {
    assert(aContext_obj->as_oop() == aContext);
    internalStoreContextRegisters(activeContext(), activeContext_obj());
    aContext_obj->beRootIfOld();
    assert(aContext != roots.nilObj);
    set_activeContext(aContext, aContext_obj);
    internalFetchContextRegisters(aContext, aContext_obj);
  }


  void internalFetchContextRegisters(Oop activeCntx, Object_p activeCntx_obj) {
    assert(activeCntx_obj->as_oop() == activeCntx);

    Object_p tmp = _localHomeContext = activeCntx_obj->home_of_block_or_method_context();

    roots.receiver =  tmp->fetchPointer(Object_Indices::ReceiverIndex);
    set_method( tmp->fetchPointer(Object_Indices::MethodIndex) );

    /*
     "the instruction pointer is a pointer variable equal to
     method oop + ip + BaseHeaderSize
     -1 for 0-based addressing of fetchByte
     -1 because it gets incremented BEFORE fetching currentByte "
     */
    oop_int_t ip_int = activeCntx_obj->quickFetchInteger(Object_Indices::InstructionPointerIndex);
    _localIP = method_obj()->as_u_char_p() + ip_int + Object::BaseHeaderSize - 2;

    // "the stack pointer is a pointer variable also..."
    oop_int_t sp_int = activeCntx_obj->quickFetchInteger(Object_Indices::StackPointerIndex);
    _localSP = (Oop*) (activeCntx_obj->as_char_p() + Object::BaseHeaderSize + (Object_Indices::TempFrameStart + sp_int - 1) * bytesPerWord);

    internalized();

    if (PrintFetchedContextRegisters) {
      dittoing_stdout_printer->printf("internalFetchContextRegisters: activeContext: ");
      activeCntx.print(dittoing_stdout_printer);
      dittoing_stdout_printer->printf(", receiver: ");
      roots.receiver.print(dittoing_stdout_printer);
      dittoing_stdout_printer->printf(", method: ");
      method().print(dittoing_stdout_printer);
      dittoing_stdout_printer->printf(", IP %d, SP %d\n", ip_int, sp_int);
    }


  }


  void internalStoreContextRegisters(Oop activeCntx, Object_p activeCntx_obj) {
    /*
     "The only difference between this method and fetchContextRegisters: is that this method stores from the local IP and SP."

     "InstructionPointer is a pointer variable equal to
     method oop + ip + BaseHeaderSize
     -1 for 0-based addressing of fetchByte
     -1 because it gets incremented BEFORE fetching currentByte"
     */
    assert(activeCntx_obj->as_oop() == activeCntx);
    activeCntx_obj->storeIntegerUnchecked_into_context(Object_Indices::InstructionPointerIndex,
                                          localIP() + 2 - (method_obj()->as_u_char_p() + Object::BaseHeaderSize) );
    activeCntx_obj->storeIntegerUnchecked_into_context(Object_Indices::StackPointerIndex,
                                          (localSP() - activeCntx_obj->as_oop_p()) - Object::BaseHeaderSize/sizeof(Oop)
                                          - Object_Indices::TempFrameStart + 1);
    registers_stored();
  }



  bool lookupInMethodCacheSel(Oop msgSel, Oop klass) {
    Method_Cache::entry* e = methodCache.at(msgSel, klass);
    if (e == NULL)
      return false;
    roots.newMethod = e->method;
    primitiveIndex = e->prim;
    roots.newNativeMethod = e->native;
    primitiveFunctionPointer = e->primFunction;
    assert(primitiveIndex == 0  ||  primitiveFunctionPointer != NULL);
    do_primitive_on_main = e->do_primitive_on_main;
    return true;
  }

  Oop lookupMethodInClass(Oop lkupClass);
  void findNewMethodInClass(Oop klass);

  void addNewMethodToCache() {
    primitiveFunctionPointer = primitiveTable.contents[primitiveIndex];
    do_primitive_on_main = primitiveTable.execute_on_main[primitiveIndex];

    methodCache.addNewMethod(roots.messageSelector, roots.lkupClass, roots.newMethod,
                             primitiveIndex, roots.newNativeMethod, primitiveFunctionPointer, do_primitive_on_main);
  }

  void internalPopThenPush(int n, Oop x) {
    set_localSP(localSP() - (n - 1));
    DEBUG_STORE_CHECK(&localSP()[0], x);
    localSP()[0] = x;
  }

  void popThenPush(int n, Oop x) { set_stackPointer(stackPointer() - (n - 1));  DEBUG_STORE_CHECK(stackPointer(), x);  *stackPointer() = x; }
  void popThenPushInteger(int n, oop_int_t i) {
    popThenPush(n, Oop::from_int(i));
  }
  void pop2AndPushIntegerIfOK(oop_int_t r) {
    if (!successFlag)
      ;
    else if (Oop::isIntegerValue(r))
      popThenPush(2, Oop::from_int(r));
    else
      successFlag = false;
  }
  double popFloat() {
    Oop top = popStack();
    assertClass(top, splObj(Special_Indices::ClassFloat));
    return successFlag  ?  top.as_object()->fetchFloatAtinto() : 0.0;
  }
  oop_int_t popInteger() {
    return popStack().checkedIntegerValue();
  }
  oop_int_t popPos32BitInteger() {
    return positive32BitValueOf(popStack());
  }

  int stackPointerIndex() {
    return stackPointer() - activeContext_obj()->as_oop_p() - Object::BaseHeaderSize/sizeof(Oop);
  }
  void run_primitive_on_main_from_elsewhere(fn_t);
  void dispatchFunctionPointer(fn_t f, bool on_main);
  void dispatchFunctionPointer(int i, Primitive_Table *pt) {
    dispatchFunctionPointer(pt->contents[i], pt->execute_on_main[i]);
  }

  bool balancedStackAfterPrimitive(int, int, int, Oop);
  void printUnbalancedStack(int, fn_t);

  void internalQuickCheckForInterrupts() {
    if (--interruptCheckCounter <= 0) {
      externalizeIPandSP();
      checkForInterrupts();
      browserPluginReturnIfNeeded();
      internalizeIPandSP();
    }
  }



  void quickCheckForInterrupts() {
    /*
     "Quick check for possible user or timer interrupts. Decrement a counter
      and only do a real check when counter reaches zero or when a low space
      or user interrupt is pending."
     "Note: Clients that trigger interrupts should set use forceInterruptCheck
      to set interruptCheckCounter to zero and get immediate results."
     "Note: Requires that instructionPointer and stackPointer be external."
     */
    if (--interruptCheckCounter <= 0) {
      checkForInterrupts();
    }
  }


  void forceInterruptCheck() {
    interruptCheckCounter = interruptCheckCounter_force_value;
    set_nextPollTick(0);
  }

  void createActualMessageTo(Oop);

  bool lookupMethodInDictionary(Oop dictionary) {
    /*
     "This method lookup tolerates integers as Dictionary keys to
     support execution of images in which Symbols have been
     compacted out"
     */
    Object_p dictionary_obj = dictionary.as_object();
    int length = dictionary_obj->fetchWordLength();
    oop_int_t mask = length - Object_Indices::SelectorStart - 1;
    int index =
    Object_Indices::SelectorStart +
    (mask &  (roots.messageSelector.is_int() ? roots.messageSelector.integerValue() : roots.messageSelector.as_object()->hashBits()));
    /*
     "It is assumed that there are some nils in this dictionary, and search
      will stop when one is encountered. However, if there are no nils, then
      wrapAround
     will be detected the second time the loop gets to the end of the table."
     */
    bool wrapAround = false;
    for (;;) {
      Oop nextSelector = dictionary_obj->fetchPointer(index);
      if (nextSelector == roots.nilObj) return false;
      if (nextSelector == roots.messageSelector)
        break;
      ++index;

      if (index == length) {
        if (wrapAround)
          return false;
        wrapAround = true;
        index = Object_Indices::SelectorStart;
      }
    }
    Oop methodArray = dictionary_obj->fetchPointer(Object_Indices::MethodArrayIndex);
    Object_p methodArray_obj = methodArray.as_object();

    if (PrintMethodDictionaryLookups) {
      dittoing_stdout_printer->printf("lookupMethodInDictionary: index = %d\n", index);
      for (int i = Object_Indices::SelectorStart;  i < length;  ++i) {
        dittoing_stdout_printer->printf("i = %d, ", i);
        dictionary_obj->fetchPointer(i).print(dittoing_stdout_printer);
        dittoing_stdout_printer->printf(", ");
        Oop m = methodArray_obj->fetchPointer(i - Object_Indices::SelectorStart);
        m.print(dittoing_stdout_printer);
        if (m.as_object()->isCompiledMethod()) {
          dittoing_stdout_printer->printf(", Obj 0x%x, firstByte: %d", (Object*)m.as_object(),
                                          *(u_char*)(m.as_object()->first_byte_address()));
        }
        dittoing_stdout_printer->nl();
      }
    }
    roots.newMethod = methodArray_obj->fetchPointer(index - Object_Indices::SelectorStart);
    // "Check if roots.newMethod is a CompiledMethod."
    Object_p nmo = newMethod_obj();
    if (!nmo->isCompiledMethod()) {
      // "indicate that this is no compiled method - use primitiveInvokeObjectAsMethod"
      primitiveIndex = 248;
    }
    else {
      primitiveIndex = nmo->primitiveIndex();
      if (primitiveIndex > Object::MaxPrimitiveIndex()) {
        /* "If primitiveIndex is out of range, set to zero before putting in
         cache. This is equiv to primFail, and avoids the need to check on
         every send."
         */
        primitiveIndex = 0;
      }
    }
    return true;
  }


  void booleanCheat(bool cond);


  int32 long_jump_offset() {
    return (((currentBytecode & 7) - 4) << 8)
    | fetchByte();
  }

  int32 long_cond_jump_offset() {
    return ((currentBytecode & 3) << 8)
    | fetchByte();
  }

  void jump(int offset) {
    set_localIP(localIP() + offset + 1);
    if (check_many_assertions)
      assert(localIP() - method_obj()->as_u_char_p()
           >= (Object_Indices::LiteralStart + Object::literalCountOfHeader(method_obj()->methodHeader())) * bytesPerWord  +  1);
    
# if Trace_Last_BC_For_Debugging
    prev_bc_for_debugging = currentBytecode;
    prev_bc_addr_for_debugging = bc_addr_for_debugging;
    bc_addr_for_debugging = localIP();
# endif
    
    currentBytecode = byteAtPointer(localIP());
    if (Check_Prefetch)  have_executed_currentBytecode = false;
  }
  void jumpIfFalseBy(int offset) {
    Oop b = internalStackTop();
    if (b == roots.falseObj)
      jump(offset);
    else if (b == roots.trueObj)
      fetchNextBytecode();
    else {
      roots.messageSelector = splObj(Special_Indices::SelectorMustBeBoolean);
      set_argumentCount(0);
      normalSend();
      return;
    }
    internalPop(1);
  }
  void jumpIfTrueBy(int offset) {
    Oop b = internalStackTop();
    if (b == roots.trueObj)
      jump(offset);
    else if (b == roots.falseObj)
      fetchNextBytecode();
    else {
      roots.messageSelector = splObj(Special_Indices::SelectorMustBeBoolean);
      set_argumentCount(0);
      normalSend();
      return;
    }
    internalPop(1);
  }
  u_char byteAtPointer(u_char* p) { return *p; }
  void checkForInterrupts(bool is_safe_to_process_events = true);

  void transfer_to_highest_priority(const char*);

  void resume(Oop, const char*);
  void yield(const char*);
  bool interruptCheckForced() {
    // was forced by outside code?
    return interruptCheckCounter  <= interruptCheckCounter_force_value;
  }
  void signalExternalSemaphores(const char*);

  void signalSema(int index, const char* why) {
    Oop sema = splObj(index);
    Safepoint_Ability sa(false);
    if (sema != roots.nilObj)
      sema.as_object()->synchronousSignal(why);
  }

  void signalSemaphoreWithIndex(int index);
  Oop schedulerPointer() {
    return splObj_obj(Special_Indices::SchedulerAssociation)
            ->fetchPointer(Object_Indices::ValueIndex);
  }
  Object_p schedulerPointer_obj() { return schedulerPointer().as_object(); }

  Object_p process_lists_of_scheduler() {
    return schedulerPointer_obj()->fetchPointer(Object_Indices::ProcessListsIndex).as_object();
  }

  Oop  get_running_process();
  void set_running_process(Oop, const char*);


  void put_running_process_to_sleep(const char*);
  Oop remove_running_process_from_scheduler_lists_and_put_it_to_sleep(const char*);

  void transferTo(Oop newProc, const char* why);
  void start_running(Oop newProc, const char*);
  Oop  find_and_move_to_end_highest_priority_non_running_process();
  int count_processes_in_scheduler();

  void newActiveContext(Oop aContext, Object_p aContext_obj);
  void commonReturn(Oop localReturnContext, Oop localReturnValue);

# if Include_Closure_Support
  Oop sender() {
    Object_p context_obj = localHomeContext();
    Oop n = roots.nilObj;
    for (;;) {
      Oop closureOrNil = context_obj->fetchPointer(Object_Indices::ClosureIndex);
      if (closureOrNil == n) break;
      context_obj = closureOrNil.as_object()->fetchPointer(Object_Indices::ClosureOuterContextIndex).as_object();
    }
    return context_obj->fetchPointer(Object_Indices::SenderIndex);
  }
# else
  Oop sender() { return localHomeContext()->fetchPointer(Object_Indices::SenderIndex); }
# endif

  Oop caller() { return activeContext_obj()->fetchPointer(Object_Indices::CallerIndex); }

  void internalCannotReturn(Oop resultObj, bool, bool, bool);

  void internalAboutToReturn(Oop resultObj, Oop aContext);


  void recycleContextIfPossible_on_its_core(Oop ctx);
  void recycleContextIfPossible_here(Oop ctx);

  bool areIntegers(Oop r, Oop a) { return r.bits() & a.bits() & Int_Tag; }

# include "interpreter_primitives.h"
# include "interpreter_bytecodes.h"


  oop_int_t doPrimitiveDiv(Oop rcvr, Oop arg);
  oop_int_t doPrimitiveMod(Oop rcvr, Oop arg);


private:
  bool roomToPushNArgs(int);
  
  
public:  



  double loadFloatOrIntFrom(Oop x) {
    if (x.is_int())  return (double)x.integerValue();
    Object_p xo = x.as_object();
    if (xo->fetchClass() == splObj(Special_Indices::ClassFloat))
      return floatValueOf(xo);
    successFlag = false;
    return 0.0;
  }

  Oop specialSelector(int index) {
    return splObj_obj(Special_Indices::SpecialSelectors)->fetchPointer(index * 2);
  }

  Logical_Core* coreWithSufficientSpaceToInstantiate(Oop klass, oop_int_t indexableSize);


  Oop commonVariableAt(Oop rcvr, oop_int_t index, At_Cache::Entry* e, bool);


  void commonVariableAtPut(Oop rcvr, oop_int_t index, Oop value, At_Cache::Entry* e);

  int32 positive32BitValueOf(Oop x);
  int32 signed32BitValueOf(Oop x);
  int64 positive64BitValueOf(Oop x);
  int64 signed64BitValueOf(Oop x);

  Oop asciiOfCharacter(Oop c) {
    assertClass(c, splObj(Special_Indices::ClassCharacter));
    return successFlag ? c.as_object()->fetchPointer(Object_Indices::CharacterValueIndex)
    : Oop::from_int(0);
  }


  void assertClass(Oop x, Oop klass) {
    if (x.is_int()) {
      successFlag = false;
      return;
    }
    assertClass(x.as_object(), klass);
  }

  void assertClass(Object_p x, Oop klass) {
    success(x->fetchClass() == klass);
  }

  void commonAt(bool);
  void commonAtPut(bool);



  Oop  stObjectAt(Object_p a, oop_int_t index);
  void stObjectAtPut(Object_p a, oop_int_t index, Oop value);


  Oop  subscript(Object_p a, oop_int_t index);
  void subscript(Object_p a, oop_int_t index, Oop value);

  void changeClass(Oop rcvr, Oop argClass, bool defer);



  oop_int_t compare31or32BitsEqual(Oop obj1, Oop obj2) {
    return
    obj1.is_int()  &&  obj2.is_int()  ?  obj1 == obj2
    :  positive32BitValueOf(obj1) == positive32BitValueOf(obj2);
  }


  void checkBooleanResult(bool b) {
    if (successFlag)  pushBool(b);
    else              unPop(2);
  }
  void executeNewMethod();


  void executeNewMethodFromCache() {
    int nArgs, delta;
    Oop pre_prim_active_context;
    if (primitiveIndex > 0) {
      Safepoint_Ability sa(true);
      if (DoBalanceChecks) {
        nArgs = get_argumentCount();
        delta = stackPointer() - activeContext_obj()->as_oop_p();
        pre_prim_active_context = activeContext();
      }
      successFlag = true;
      dispatchFunctionPointer(primitiveFunctionPointer, do_primitive_on_main);
      // branch direct to prim fn
      if (DoBalanceChecks  &&  !balancedStackAfterPrimitive(delta, primitiveIndex, nArgs, pre_prim_active_context))
        printUnbalancedStack(primitiveIndex, primitiveFunctionPointer);
      if (successFlag) return;
    }
    activateNewMethod();
    quickCheckForInterrupts();
  }


  bool primitiveResponse();
  void primitivePerformAt(Oop);

  Oop characterForAscii(char c) {
    return splObj_obj(Special_Indices::CharacterTable)->fetchPointer(u_char(c));
  }


  void transferFromIndexOfObjectToIndexOfObject(oop_int_t count,
                                                oop_int_t firstFrom, Object_p fromObj,
                                                oop_int_t firstTo,   Object_p   toObj) {
    // assume beRootIfOld: will be called on toOop
    static const int offset = Object::BaseHeaderSize / sizeof(Oop);
    oopcpy_no_store_check(toObj->as_oop_p() + firstTo + offset,  fromObj->as_oop_p() + firstFrom + offset, count, toObj);
  }

  void snapshot(bool);
  void snapshotCleanUp();

  void     displayBitsOf(Oop, oop_int_t, oop_int_t, oop_int_t, oop_int_t);
  void showDisplayBitsOf(Oop, oop_int_t, oop_int_t, oop_int_t, oop_int_t);
  void fullDisplayUpdate();

  void print_stack_trace(Printer*, Object_p proc = (Object_p)NULL);
  void print_all_stack_traces(Printer*);
  void print_process_lists(Printer*);
  void print_all_processes_in_scheduler(Printer*, bool);
  bool verify_all_processes_in_scheduler();
  void print_all_instances_of_class_process(Printer*, bool);
  void print_all_processes_in_scheduler_or_on_a_list(Printer*, bool);
  bool is_process_on_a_scheduler_list(Oop);

  void copyBits();
  void copyBitsFromtoat(oop_int_t, oop_int_t, oop_int_t);

  oop_int_t ioFilenamefromStringofLengthresolveAliases(char* aCharBuffer, char* filenameIndex, oop_int_t filenameLength, oop_int_t resolveFlag);



  Oop splObj(oop_int_t i) {
    return roots.specialObjectsOop.as_object()->fetchPointer(i);
  }

  Object_p splObj_obj(oop_int_t i) {
    return splObj(i).as_object();
  }




  Object_p classString()   { return splObj_obj(Special_Indices::ClassString); }

  Oop     displayObject();


  Object_p allocateOrRecycleContext(bool needsLarge);


  bool verify();

  Oop get_stats(int);

  int makeArrayStart() { return remapBufferCount; }
  Oop makeArray(int start);

# define PUSH_FOR_MAKE_ARRAY(expr) \
  (The_Squeak_Interpreter()->pushRemappableOop(expr))

# define PUSH_STRING_FOR_MAKE_ARRAY(expr)  \
  PUSH_FOR_MAKE_ARRAY(Object::makeString(expr)->as_oop())

# define PUSH_POSITIVE_32_BIT_INT_FOR_MAKE_ARRAY(expr)  \
  PUSH_FOR_MAKE_ARRAY(Object::positive32BitIntegerFor(expr))

# define PUSH_POSITIVE_64_BIT_INT_FOR_MAKE_ARRAY(expr)  \
  PUSH_FOR_MAKE_ARRAY(Object::positive64BitIntegerFor(expr))

# define PUSH_BOOL_FOR_MAKE_ARRAY(expr)  \
  PUSH_FOR_MAKE_ARRAY((expr) ? The_Squeak_Interpreter()->roots.trueObj : The_Squeak_Interpreter()->roots.falseObj)



# define PUSH_WITH_STRING_FOR_MAKE_ARRAY(expr)  \
  (PUSH_FOR_MAKE_ARRAY(Object::makeString(#expr)->as_oop()), \
   PUSH_FOR_MAKE_ARRAY(expr))

# define PUSH_POSITIVE_32_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(expr)  \
  ( PUSH_FOR_MAKE_ARRAY(Object::makeString(#expr)->as_oop()), \
    PUSH_FOR_MAKE_ARRAY(Object::positive32BitIntegerFor(expr)))

# define PUSH_POSITIVE_64_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(expr)  \
  ( PUSH_FOR_MAKE_ARRAY(Object::makeString(#expr)->as_oop()), \
    PUSH_FOR_MAKE_ARRAY(Object::positive64BitIntegerFor(expr)))

# define PUSH_BOOL_WITH_STRING_FOR_MAKE_ARRAY(expr)  \
( PUSH_FOR_MAKE_ARRAY(Object::makeString(#expr)->as_oop()), \
  PUSH_BOOL_FOR_MAKE_ARRAY((expr)))

 private:
  bool verify_active_context_through_internal_stack_top();
  void let_one_through();
 public:
  // TODO: Rename baton related functions into something like: currently_interpreting
  void release_baton();
  bool do_I_hold_baton();

 private:
  void check_for_multicore_interrupt() {
    assert(multicore_interrupt_check  ||  do_I_hold_baton());
    // xxxxxx If set multicore_interrupt_check whenever yield_requested() 
    //        will be true, could speed up this test.
    // -- dmu 4/09
    if (multicore_interrupt_check || yield_requested() || Message_Queue::are_data_available(my_core()))
       multicore_interrupt();
  }

  void multicore_interrupt();
  void try_to_find_a_process_to_run_and_start_running_it();
  void minimize_scheduler_mutex_load_by_spinning_till_there_might_be_a_runnable_process();
  void give_up_CPU_instead_of_spinning(uint32_t&);
  void fixup_localIP_after_being_transferred_to();
 private:
  void move_mutated_read_mostly_objects();

  bool is_ok_to_run_on(int rank) {
    return ((1LL << u_int64(rank)) & run_mask()) ? true : false;
  }
  bool is_ok_to_run_on_me() {
    return (Logical_Core::my_rank_mask() & run_mask()) ? true : false;
  }


  void update_times_when_resuming();
  void update_times_when_yielding();
  void update_times_when_asking();

  void save_to_checkpoint(FILE*);
  void restore_from_checkpoint(FILE*);
 public:
  void save_all_to_checkpoint();
  void restore_all_from_checkpoint(int dataSize, int lastHash, int savedWindowSize, int fullScreenFlag);

  void trace_execution();
  void print_execution_trace();
  void trace_for_debugging();

  void assert_method_is_correct(bool will_be_fetched, const char* where) {
    if (Always_Check_Method_Is_Correct ||  check_assertions) check_method_is_correct(will_be_fetched, where);
  }
  
  void assert_method_is_correct_internalizing(bool will_be_fetched, const char* where) {
    if ((Always_Check_Method_Is_Correct  || check_assertions) && do_I_hold_baton())
      assert_always_method_is_correct_internalizing(will_be_fetched, where);
  }
  
  void assert_always_method_is_correct_internalizing(bool will_be_fetched, const char* where) {
    if (do_I_hold_baton()) {
      Safepoint_Ability sa(false);
      internalizeIPandSP();
      check_method_is_correct(will_be_fetched, where);
    }
  }
  void check_method_is_correct(bool, const char*); // for debugging

  void undo_prefetch() {
    if (Check_Prefetch) assert_always(!have_executed_currentBytecode);
    set_instructionPointer(instructionPointer() - 1); // send bytecode incremented this in fetchNextBytecode, but we want original value so yield can store it
    if (Check_Prefetch) have_executed_currentBytecode = true;
  }

  void internal_undo_prefetch() {
    if (Check_Prefetch) assert_always(!have_executed_currentBytecode);
    set_localIP(localIP() - 1);  // "undo the pre-increment of IP before returning"
    if (Check_Prefetch) have_executed_currentBytecode = true;
  }

  void signalFinalization(Oop);

  void handle_sigint();
  void print_info();
  void signal_emergency_semaphore();

private:
  int literal_index_of_bytecode(u_char*);
  
  
  oop_int_t compute_primitive_index(Object_p lo);
  
  bool lookup_in_externalPrimitiveTable(Object_p lo);
  
  void fetch_module_and_fn_name(Object_p lo, 
                                Oop& moduleName, Object_p& mno, int& moduleLength,
                                Oop& functionName, Object_p& fno, int& functionLength);
  void lookup_in_obsoleteNamedPrimitiveTable(Oop functionName, Object_p& fno, int functionLength,
                                             Oop  moduleName, Object_p& mno, int moduleLength,
                                             bool& on_main, fn_t& addr);
  fn_t munge_arguments_and_load_function_from_plugin_on_main(Oop functionName, Object_p& fno, int functionLength,
                                                             Oop  moduleName, Object_p& mno, int moduleLength);
  void update_cache_and_call_external_function(Object_p fno, oop_int_t ii, fn_t addr, bool on_main);  
  void update_cache_and_report_failure_to_load_external_function(Object_p mno, Object_p fno);  
  
private:
  void broadcast_u_int64(u_int64*);
  void broadcast_int32(int32* w);
  void broadcast_bool(bool* b);
private:
  void broadcast_datum(int size, void* p, u_int64 d);
  


public:
  Safepoint_Ability *safepoint_ability;

  void distribute_initial_interpreter();
  void receive_initial_interpreter();
  
# if Dump_Bytecode_Cycles
  u_int64 bc_cycles[10000];
  u_int64 bc_cycles_tare;
  int bc_cycles_index;
# endif
  
# if Track_Last_BC_For_Debugging
  u_char  prev_bc_for_debugging;
  u_char* prev_bc_addr_for_debugging;
  u_char* bc_addr_for_debugging;  
# endif
};






# define FOR_EACH_READY_PROCESS_LIST(slo, pri, list_obj, interp) /* highest to lowest priority */ \
/* pri must be an int or oop_int_t, list_obj must be an object* */ \
Object_p slo = interp->process_lists_of_scheduler(); \
Object_p list_obj; \
oop_int_t pri; \
for ( pri = slo->fetchWordLength() - 1;  \
     pri >= 0  &&  (list_obj = slo->fetchPointer(pri).as_object());  \
--pri)

