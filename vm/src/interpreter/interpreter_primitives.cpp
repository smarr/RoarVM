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

void Squeak_Interpreter::primitiveAdd() {
  pop2AndPushIntegerIfOK(stackIntegerValue(1) + stackIntegerValue(0));
}

# include <math.h>

void Squeak_Interpreter::primitiveArctan() {
  double r = popFloat();
  if (successFlag) pushFloat(atan(r));
  else unPop(1);
}

#warning STEFAN: I fear there is something broken in the overall implementation of primitives here, primitiveArrayBecome makes problems now, with optimization level > 1, I expect the others also to be volatile...
__attribute__((optimize(1)))
void Squeak_Interpreter::primitiveArrayBecome() {
  success(The_Memory_System()->become_with_twoWay_copyHash(stackValue(1), stackTop(), true, true));
  if (successFlag) pop(1);
  // lprintf("primitiveArrayBecome\n"); 
}

void Squeak_Interpreter::primitiveArrayBecomeOneWay() {
  success(The_Memory_System()->become_with_twoWay_copyHash(stackValue(1), stackTop(), false, true));
  if (successFlag) pop(1);
  // lprintf("primitiveArrayBecomeOneWay\n");
}

void Squeak_Interpreter::primitiveArrayBecomeOneWayCopyHash() {
  success(The_Memory_System()->become_with_twoWay_copyHash(stackValue(2), stackValue(1), false, booleanValueOf(stackTop())));
  if (successFlag) pop(2);
  // lprintf("primitiveArrayBecomeOneWayCopyHash\n");
}


void Squeak_Interpreter::primitiveAsFloat() {
  oop_int_t a = popInteger();
  if (successFlag)
    pushFloat(double(a));
  else
    unPop(1);
}

void Squeak_Interpreter::primitiveAsOop() {
  Oop r = stackTop();
  success(r.is_mem());
  if (successFlag)  popThenPushInteger(1, r.as_object()->hashBits());
}

void Squeak_Interpreter::primitiveAt() {
  commonAt(false);
}

void Squeak_Interpreter::primitiveAtEnd() {
  Oop stream = popStack();
  Object_p so;
  successFlag = stream.is_mem() && (so = stream.as_object())->isPointers()
  && so->lengthOf() >= Object_Indices::StreamReadLimitIndex + 1;
  if (successFlag) {
    oop_int_t index = so->fetchInteger(Object_Indices::StreamIndexIndex);
    oop_int_t limit = so->fetchInteger(Object_Indices::StreamReadLimitIndex);
    if (successFlag)  {
      pushBool(index >= limit);
      return;
    }
    unPop(1);
  }
}

void Squeak_Interpreter::primitiveAtPut() {
  commonAtPut(false);
}

void Squeak_Interpreter::primitiveBeCursor() {
  Oop cursorObj, maskObj;
  char* maskBitsIndex;

  if (get_argumentCount() == 0) {
    cursorObj = stackTop();
    maskBitsIndex = NULL;
  }
  else if (get_argumentCount() == 1) {
    cursorObj = stackValue(1);
    maskObj = stackTop();
  }
  success(get_argumentCount() < 2);

  Object_p co;
  success(cursorObj.is_mem() && (co = cursorObj.as_object())->lengthOf() >= 5);
  Oop bitsObj;
  Object_p bo;
  Object_p oo;
  oop_int_t extentX, extentY, depth;
  if (successFlag) {
    bitsObj = co->fetchPointer(0);
    extentX = co->fetchInteger(1);
    extentY = co->fetchInteger(2);
    depth   = co->fetchInteger(3);
    Oop offsetObj = co->fetchPointer(4);
    success(offsetObj.is_mem()  &&  (oo = offsetObj.as_object())->lengthOf() >= 2);
    success(bitsObj.is_mem()  &&  (bo = bitsObj.as_object()));
  }
  oop_int_t offsetX, offsetY;
  char* cursorBitsIndex;
  if (successFlag) {
    offsetX = oo->fetchInteger(0);
    offsetY = oo->fetchInteger(1);
    success(extentX == 16  &&  extentY == 16  &&  depth == 1);
    success(offsetX >= -16  &&  offsetX <= 0);
    success(offsetY >= -16  &&  offsetY <= 0);
    cursorBitsIndex = bo->as_char_p() + Object::BaseHeaderSize;
  }
  if (get_argumentCount() == 1) {
    Object_p mo;
    success(maskObj.is_mem() && (mo = maskObj.as_object())->lengthOf() >= 5);
    if (successFlag) {
      bitsObj = mo->fetchPointer(0);
      extentX = mo->fetchInteger(1);
      extentY = mo->fetchInteger(2);
      depth = mo->fetchInteger(3);
    }
    if (successFlag) {
      success(extentX == 16  &&  extentY == 16  &&  depth == 1);
      success(bitsObj.is_mem()  &&  (bo = bitsObj.as_object())  && bo->lengthOf() == 16);
      maskBitsIndex = bo->as_char_p() + Object::BaseHeaderSize;
    }
  }
  if (successFlag) {
    if (get_argumentCount() == 0)
      ioSetCursor(cursorBitsIndex, offsetX, offsetY);
    else
      ioSetCursorWithMask(cursorBitsIndex, maskBitsIndex, offsetX, offsetY);
    pop(get_argumentCount());
  }
}

void Squeak_Interpreter::primitiveBeDisplay() {
  Oop rcvr;
  rcvr = stackTop();
  success(rcvr.is_mem() &&  rcvr.as_object()->lengthOf() >= 4);
  if (successFlag)
    roots.specialObjectsOop.as_object()->storePointer(Special_Indices::TheDisplay, rcvr);
}

void Squeak_Interpreter::primitiveBeep() {
  untested();
  ioBeep();
}

void Squeak_Interpreter::primitiveBitAnd() {
  oop_int_t a = popPos32BitInteger(), r = popPos32BitInteger();
  if (successFlag)
    push(Object::positive32BitIntegerFor(r & a));
  else
    unPop(2);
}

void Squeak_Interpreter::primitiveBitOr() {
  oop_int_t a = popPos32BitInteger(), r = popPos32BitInteger();
  if (successFlag)
    push(Object::positive32BitIntegerFor(r | a));
  else
    unPop(2);
}

void Squeak_Interpreter::primitiveBitShift() {
  oop_int_t a = popInteger();
  u_oop_int_t shifted;
  u_oop_int_t r = popPos32BitInteger();
  if (successFlag) {
    if (a >= 0) {
      success(a <= 31);
      shifted = r << a;
      success((shifted >> a) == r);
    }
    else {
      success(a >= -31);
      shifted = r >> -a;
    }
  }
  if (successFlag)  push(Object::positive32BitIntegerFor(shifted));
  else unPop(2);
}

void Squeak_Interpreter::primitiveBitXor() {
  oop_int_t a = popPos32BitInteger(), r = popPos32BitInteger();
  if (successFlag)
    push(Object::positive32BitIntegerFor(r ^ a));
  else
    unPop(2);
}

void Squeak_Interpreter::primitiveBlockCopy() {
  Oop methodContext;
  oop_int_t contextSize;
  {
    Oop context = stackValue(1); Object_p co = context.as_object();
    methodContext = co->home_of_block_or_method_context()->as_oop();
    assert(methodContext.as_object()->isMethodContext());
    contextSize = methodContext.as_object()->sizeBits(); // in bytes, incl header
  }
  pushRemappableOop(methodContext);
  Object_p nco = splObj_obj(Special_Indices::ClassBlockContext)->instantiateContext(contextSize);
  methodContext = popRemappableOop();

  oop_int_t initialIP = (instructionPointer() + 1 + 3) - (method_obj()->as_u_char_p() + Object::BaseHeaderSize);
  // Was instructionPointer + 3, but now greater by 1 due to preinc

  // Assume new context is young, so use unchecked stores
  nco->storeIntegerUnchecked_into_context(Object_Indices::InitialIPIndex, initialIP);
  nco->storeIntegerUnchecked_into_context(Object_Indices::InstructionPointerIndex, initialIP);
  nco->storeStackPointerValue(0);
  nco->storePointerIntoYoung(Object_Indices::BlockArgumentCountIndex, stackValue(0));
  nco->storePointerIntoYoung(Object_Indices::HomeIndex, methodContext);
  nco->storePointerIntoYoung(Object_Indices::SenderIndex, roots.nilObj);

  nco->save_block_method_and_IP();

  popThenPush(2, nco->as_oop());
 }

void Squeak_Interpreter::primitiveBytesLeft() {
  untested();
  /*
   Reports bytes available at this moment. For more meaningful
   results, calls to this primitive should be preceeded by a full
   or incremental garbage collection
   */
  switch (methodArgumentCount()) {
    case 0:
      popThenPushInteger(1, The_Memory_System()->bytesLeft(false));
      return;

    case 1: {
      bool swapSpace = booleanValueOf(stackTop());
      if (!successFlag) return;
      popThenPushInteger(2, The_Memory_System()->bytesLeft(swapSpace));
      return;
    }
    default:
      primitiveFail();
      return;
  }
}

void Squeak_Interpreter::primitiveCalloutToFFI() {

  untested();
  /*
   Perform a function call to a foreign function.
   Only invoked from method containing explicit external call spec.
   Due to this we use the pluggable prim mechanism explicitly here
   (the first literal of any FFI spec'ed method is an ExternalFunction
   and not an array as used in the pluggable primitive mechanism)."
   */
  // Stefan: this Callout function just loads the callout primitive when it is
  //         used for the first time, should be no problem to share it,
  //         concurrent initialization does not pose a risk as far as i can see
  static fn_t function = NULL;    // should be safe

  static const char* moduleName = "SqueakFFIPrims";
  static const char* functionName = "primitiveCallout";
  assert_on_main();
  if (function == NULL) {
    function = ioLoadExternalFunctionOfLengthFromModuleOfLength((char*)functionName, 16, (char*)moduleName, 14);
    if (function == NULL) {
      primitiveFail();
      return;
    }
  }
  (*function)();
}

void Squeak_Interpreter::primitiveChangeClass() {
  /*
   "Primitive. Change the class of the receiver into the class of the argument
    given that the format of the receiver matches the format of the argument's
    class. Fail if receiver or argument are SmallIntegers, or the receiver is
    an instance of a compact class and the argument isn't, or when the
    argument's class is compact and the receiver isn't, or when the format of
    the receiver is different from the format of the argument's class, or when
    the arguments class is fixed and the receiver's size differs from the size
    that an instance of the argument's class should have."
   */
  if (methodArgumentCount() != 1) {
    primitiveFail();
    return;
  }

  Oop arg = stackObjectValue(0);
  Oop rcvr = stackObjectValue(1);
  Oop argClass = arg.fetchClass();
  changeClass(rcvr, argClass, true);
  if (successFlag)  pop(1);
}

void Squeak_Interpreter::primitiveClass() {
  popThenPush(get_argumentCount() + 1, stackTop().fetchClass());
}

void Squeak_Interpreter::primitiveClipboardText() {
  /*When called with a single string argument, post the string to
   the clipboard. When called with zero arguments, return a
   string containing the current clipboard contents."*/
  if (get_argumentCount() == 1) {
    Oop s = stackTop();
    Object_p so;
    if (!s.is_mem()  || !(so = s.as_object())->isBytes()) {
      primitiveFail(); return;
    }
    if (successFlag) {
      clipboardWriteFromAt(so->stSize(), so->as_char_p() + Object::BaseHeaderSize, 0);
      pop(1);
    }
  }
  else {
    oop_int_t sz = clipboardSize();
    if (The_Memory_System()->coreWithSufficientSpaceToAllocate(sz, Memory_System::read_write) == NULL) {
      primitiveFail();  return;
    }
    Oop s = classString()->instantiateClass(sz)->as_oop();
    Object_p so = s.as_object();
    char* start = so->as_char_p() + Object::BaseHeaderSize;
    assert(The_Memory_System()->contains(start));

    The_Memory_System()->enforce_coherence_before_store_into_object_by_interpreter(start, sz, so);
    clipboardReadIntoAt(sz, 0, start);
    The_Memory_System()->enforce_coherence_after_store_into_object_by_interpreter(start, sz);
    popThenPush(1, s);
  }
}

void Squeak_Interpreter::primitiveClone() {
  Oop x = stackTop();
  if (x.is_int())
    return;
  Oop nc = x.as_object()->clone();
  if (nc.is_int()) { primitiveFail(); return; }
  popThenPush(1, nc);
  assert(nc.fetchClass() != roots.nilObj);
}

void Squeak_Interpreter::primitiveConstantFill() {
  // rcvr is indexable bytes or words
  oop_int_t fillValue = positive32BitValueOf(stackTop());
  Oop rcvr = stackValue(1);
  Object_p ro;
  success(rcvr.is_mem()  && (ro = rcvr.as_object())->isWordsOrBytes());
  if (!successFlag) return;
  bool isB = ro->isBytes();
  if (isB) success((fillValue & ~0xff) == 0);
  if (!successFlag) return;

  char* start = ro->as_char_p()  + Object::BaseHeaderSize;
  int bytes = ro->sizeBits() - Object::BaseHeaderSize;

  assert(The_Memory_System()->contains(start));

  The_Memory_System()->enforce_coherence_before_store_into_object_by_interpreter(start, bytes, ro);
  DEBUG_MULTISTORE_CHECK( (Oop*)start, Oop::from_bits(fillValue), bytes >> ShiftForWord);
  if (isB) memset(         start, fillValue,  bytes);
  else     wordset((int32*)start, fillValue,  bytes >> ShiftForWord);
  The_Memory_System()->enforce_coherence_after_store_into_object_by_interpreter(start, bytes);

  pop(1);
}

void Squeak_Interpreter::primitiveCopyObject() {
  untested();
  /*
   "Primitive. Copy the state of the receiver from the argument.
   Fail if receiver and argument are of a different class.
   Fail if the receiver or argument are non-pointer objects.
   Fail if receiver and argument have different lengths (for indexable objects).
   "*/
  if (methodArgumentCount() != 1) { primitiveFail(); return; }
  Oop arg = stackObjectValue(0);
  Oop rcvr = stackObjectValue(1);

  if (failed()) return;
  Object_p ro;
  Object_p ao;
  if (!rcvr.is_mem() || !arg.is_mem()) { primitiveFail(); return; }
  ro = rcvr.as_object();  ao = arg.as_object();
  if (ro->fetchClass() != ao->fetchClass()) { primitiveFail(); return; }
  oop_int_t length = ro->lengthOf();

  for (int i = 0;  i < length;  ++i)
    ro->storePointer(i, ao->fetchPointer(i));

  pop(1);
}

void Squeak_Interpreter::primitiveDeferDisplayUpdates() {
   Oop flag = stackTop();
  if (flag == roots.trueObj)
    set_deferDisplayUpdates(true);
  else if (flag == roots.falseObj)
    set_deferDisplayUpdates(false);
  else {
    primitiveFail(); return;
  }
  pop(1);
}

void Squeak_Interpreter::primitiveDiv() {
  pop2AndPushIntegerIfOK( doPrimitiveDiv(stackValue(1), stackTop()));
}

void Squeak_Interpreter::primitiveDivide() {
  oop_int_t ir = stackIntegerValue(1), ia = stackIntegerValue(0);
  if (ia != 0  &&  ir % ia == 0)
    pop2AndPushIntegerIfOK(ir / ia);
  else
    primitiveFail();
}

void Squeak_Interpreter::primitiveDoPrimitiveWithArgs() {
  Oop argumentArray = stackTop();
  if (!argumentArray.is_mem()) { primitiveFail();  return; }
  Object_p aao = argumentArray.as_object();
  oop_int_t arraySize = aao->fetchWordLength();
  oop_int_t cntxSize = activeContext_obj()->fetchWordLength();
  success(stackPointerIndex() + arraySize  <  cntxSize);
  if (!aao->isArray()) {  primitiveFail();  return; }

  int primIdx = stackIntegerValue(1);
  if (!successFlag)  {  primitiveFail();  return; }

  // pop prim index and argArray, push args
  pop(2);
  primitiveIndex = primIdx;
  set_argumentCount( arraySize );
  for( oop_int_t index = 1;  index <= arraySize;  ++index )
    push( aao->fetchPointer(index - 1) );

  // Run the prim (sets successFlag)
  pushRemappableOop(argumentArray);
  roots.lkupClass = roots.nilObj;
  primitiveResponse();
  aao = NULL; // GC recovery
  argumentArray = popRemappableOop();
  if (!successFlag) {
    // restore state
    pop(arraySize);
    pushInteger(primIdx);
    push(argumentArray);
    set_argumentCount( 2 );
  }
}

void Squeak_Interpreter::primitiveDoNamedPrimitiveWithArgs() {
  /* "Simulate an primitiveExternalCall invocation (e.g. for the Debugger).
      Do not cache anything.
      e.g. ContextPart>>tryNamedPrimitiveIn: aCompiledMethod for: aReceiver withArgs: arguments"
  */
  
  Oop argumentArray = stackTop();
  if (!argumentArray.isArray()) { primitiveFail();  return; }
  Object_p argumentArray_obj = argumentArray.as_object();
  
  oop_int_t arraySize = argumentArray_obj->fetchWordLength();
  success( roomToPushNArgs( arraySize));
  
  Oop methodArg = stackObjectValue(2);
  if ( !successFlag ) { primitiveFail(); return; }
  Object_p methodArg_object = methodArg.as_object();
  if (!methodArg_object->isCompiledMethod()) { primitiveFail(); return; }
  
  int methodHeader = methodArg_object->methodHeader();
  
  if (Object::literalCountOfHeader(methodHeader) <= 2) { primitiveFail(); return; }
  Oop spec = methodArg_object->fetchPointer(1); // first literal
  assertClass(spec, splObj(Special_Indices::ClassArray));
  if (!successFlag) return;
  Object_p spec_obj = spec.as_object();
  if ( spec_obj->lengthOf() != 4  
      ||  Object::primitiveIndex_of_header(methodHeader) != 117
      ||  Object::argumentCountOfHeader(methodHeader) != arraySize) {
    primitiveFail(); return; 
  }
  // Function not loaded yet. Fetch module & func name
  Oop moduleName = spec_obj->fetchPointer(0);
  if (!moduleName.is_mem()) { primitiveFail(); return; }
  Object_p moduleName_obj = moduleName.as_object();
  int moduleLength = moduleName == roots.nilObj  ?  0  :  moduleName_obj->lengthOf();
  
  Oop functionName = spec_obj->fetchPointer(1);
  success( functionName.isBytes() );
  if (!successFlag) return;
  Object_p functionName_obj = functionName.as_object();
  int functionLength = functionName_obj->lengthOf();
  
  fn_t addr;
  {
    Safepoint_Ability sa1(true); // load may need to wait for main
    addr = munge_arguments_and_load_function_from_plugin_on_main(functionName, functionName_obj, functionLength, moduleName, moduleName_obj, moduleLength);
    
    // recover from GC
    argumentArray_obj = argumentArray.as_object();
    methodArg_object = methodArg.as_object();
    spec_obj = spec.as_object();
  }
  if (addr == NULL) { primitiveFail(); return; }
  
  // cannot fail this prim, could fail the external prim
  pop(1);
  set_argumentCount(arraySize);
  for (int i = 1;  i <= arraySize;  ++i)
    push(argumentArray_obj->fetchPointer(i - 1));
  
  // run the prim
  pushRemappableOop(argumentArray); // might alloc
  roots.lkupClass = roots.nilObj;
  dispatchFunctionPointer(addr, true);
  argumentArray = popRemappableOop();
  argumentArray_obj = argumentArray.as_object();
  
  if (!successFlag) {
    popThenPush(arraySize, argumentArray);
    set_argumentCount(3);
  }
  
}

void Squeak_Interpreter::primitiveEqual() {
  checkBooleanResult(compare31or32BitsEqual(popStack(), popStack()));
}
void Squeak_Interpreter::primitiveEquivalent() {
  pushBool(popStack() == popStack());
}
void Squeak_Interpreter::primitiveExecuteMethod() {
  untested();
  roots.newMethod = popStack();
  if (!roots.newMethod.is_mem()) {
    primitiveFail();
    unPop(1);
    return;
  }
  Object_p nmo = newMethod_obj();
  primitiveIndex = nmo->primitiveIndex();
  success(get_argumentCount() - 1  ==  nmo->argumentCount());
  if (successFlag)
      executeNewMethod();
  else
    unPop(1);
}

void Squeak_Interpreter::primitiveExecuteMethodArgsArray() {
  roots.newMethod = popStack();
  if (!roots.newMethod.is_mem()) {
    unPop(1);
    primitiveFail();
    return;
  }
  Object_p nmo = newMethod_obj();
  primitiveIndex = nmo->primitiveIndex();
  int argCnt = nmo->argumentCount();
  Oop argumentArray = popStack();
  Object_p aao;
  if (!argumentArray.is_mem()  ||  !(aao = argumentArray.as_object())->isArray()) {
    unPop(2);
    primitiveFail();
    return;
  }
  if (successFlag)
    success(argCnt == aao->fetchWordLength());
  if (successFlag) {
    oopcpy_no_store_check(stackPointer() + 1,
                          aao->as_oop_p() + Object::BaseHeaderSize/sizeof(Oop),
                          argCnt,
                          activeContext_obj());
    unPop(argCnt);
    set_argumentCount( argCnt );
    executeNewMethod();
  }
  else
    unPop(2);
}

void Squeak_Interpreter::primitiveExitToDebugger() {
  untested();
  fatal("you asked for it");
}

void Squeak_Interpreter::primitiveExponent() {
  double r = popFloat();
  if (!successFlag) unPop(1);
  else {
    int pwr;
    frexp(r, &pwr);
    pushInteger(pwr - 1);
  }
}
void Squeak_Interpreter::primitiveExp() {
  double r = popFloat();
  if (successFlag)
    pushFloat(exp(r));
  else
    unPop(1);
}



fn_t last_external_call_fn[Memory_Semantics::max_num_threads_on_threads_or_1_on_processes]; // for debugging



void Squeak_Interpreter::primitiveExternalCall() {
  /*
   Call an external primitive. The external primitive methods
   contain as first literal an array consisting of:
   * The module name (String | Symbol)
   * The function name (String | Symbol)
   * The session ID (SmallInteger) [OBSOLETE]
   * The function index (Integer) in the externalPrimitiveTable
   For fast failures the primitive index of any method where the
   external prim is not found is rewritten in the method cache
   with zero. This allows for ultra fast responses as long as the
   method stays in the cache.
   The fast failure response relies on roots.lkupClass being properly
   set. This is done in
   #addToMethodCacheSel:class:method:primIndex: to
   compensate for execution of methods that are looked up in a
   superclass (such as in primitivePerformAt).
   With the latest modifications (e.g., actually flushing the
   function addresses from the VM), the session ID is obsolete.
   But for backward compatibility it is still kept around. Also, a
   failed lookup is reported specially. If a method has been
   looked up and not been found, the function address is stored
   as -1 (e.g., the SmallInteger -1 to distinguish from
   16rFFFFFFFF which may be returned from the lookup).
   It is absolutely okay to remove the rewrite if we run into any
   problems later on. It has an approximate speed difference of
   30% per failed primitive call which may be noticable but if,
   for any reasons, we run into problems (like with J3) we can
   always remove the rewrite.
   */

  // fetch first literal
  Safepoint_Ability sa(false);
  Object_p lo = newMethod_obj()->get_external_primitive_literal_of_method();
  success(lo != NULL);
  if (!successFlag) return;

  if (lookup_in_externalPrimitiveTable(lo)) return;
  // Recover from GC
  lo = newMethod_obj()->get_external_primitive_literal_of_method();

  lo->cleanup_session_ID_and_ext_prim_index_of_external_primitive_literal();

  // fn not loaded yet, fetch module and fn name
  Oop moduleName, functionName;
  Object_p mno;
  Object_p fno;
  int moduleLength, functionLength;
  fetch_module_and_fn_name(lo, moduleName, mno, moduleLength, functionName, fno, functionLength);
  if (!successFlag) return;

  // attempt to map old-style
  fn_t addr = NULL;
  bool on_main = false;
  lookup_in_obsoleteNamedPrimitiveTable(functionName, fno, functionLength, 
                                        moduleName, mno, moduleLength,
                                        on_main, addr);
  
  if (addr == NULL) {
    Safepoint_Ability sa1(true); // load may need to wait for main
    addr = munge_arguments_and_load_function_from_plugin_on_main(functionName, fno, functionLength, moduleName, mno, moduleLength);
    // recover from GC
    lo = newMethod_obj()->get_external_primitive_literal_of_method();
    fno = functionName.as_object();
    mno = moduleName.as_object();

    //lo = newMethod_obj()->get_external_primitive_literal_of_method();

    on_main = true; // conservative, can correct individuals by added them to obsoleteNamedPrimitiveTable
  }

  oop_int_t ii =  addr == NULL  ?  Abstract_Primitive_Table::lookup_failed  :  externalPrimitiveTable()->add(addr, on_main);
  success(Abstract_Primitive_Table::is_index_valid(ii));
  lo->storeInteger(Object_Indices::EPL_External_Primitive_Table_Index, ii);

  if (successFlag &&  addr != NULL) {
    update_cache_and_call_external_function(fno, ii, addr, on_main);
    // recover from GC
    lo = newMethod_obj()->get_external_primitive_literal_of_method();
    fno = functionName.as_object();
    mno = moduleName.as_object();
  }
  else
    update_cache_and_report_failure_to_load_external_function(mno, fno);
}


oop_int_t Squeak_Interpreter::compute_primitive_index(Object_p lo) {
  Oop index = lo->fetchPointer(Object_Indices::EPL_External_Primitive_Table_Index);
  oop_int_t ii = index.checkedIntegerValue();
  if (!successFlag) return Abstract_Primitive_Table::lookup_failed;
  if (ii != Abstract_Primitive_Table::lookup_failed)
    return ii;
    
    // fn addr not found, rewrite mcache
    methodCache.rewrite(roots.messageSelector, roots.lkupClass, 0);
  success(false);
  return Abstract_Primitive_Table::lookup_failed;
}



bool Squeak_Interpreter::lookup_in_externalPrimitiveTable(Object_p lo) {
  oop_int_t ii = compute_primitive_index(lo);
  if (!successFlag) 
    return true;
  if (!Abstract_Primitive_Table::is_index_valid(ii)  ||  ii > MaxExternalPrimitiveTableSize)
    return false;
  
  fn_t addr = externalPrimitiveTable()->contents[ii - 1];
  bool on_main = externalPrimitiveTable()->execute_on_main[ii - 1];
  if (addr != NULL) {
    methodCache.rewrite(roots.messageSelector, roots.lkupClass, 1000 + ii, addr, on_main);
    last_external_call_fn[rank_on_threads_or_zero_on_processes()] = addr;
    dispatchFunctionPointer(addr, on_main);
    return true;
  }
  // index was kept on ST side though prim table was flushed
  lprintf("prim table rep issue?\n");
  primitiveFail();
  return true;
}

void Squeak_Interpreter::fetch_module_and_fn_name(Object_p lo, 
                                                  Oop& moduleName, Object_p& mno, int& moduleLength,
                                                  Oop& functionName, Object_p& fno, int& functionLength) {
  // fn not loaded yet, fetch module and fn name
  moduleName = lo->fetchPointer(Object_Indices::EPL_Module_Name);
  if (!moduleName.is_mem()) { primitiveFail(); return; }
  mno = moduleName.as_object();
  if (moduleName == roots.nilObj)
    moduleLength = 0;
  else {
    success(mno->isBytes());
    moduleLength = mno->lengthOf();
  }
  
  functionName = lo->fetchPointer(Object_Indices::EPL_Function_Name);
  if (!functionName.is_mem()) { primitiveFail(); return; }
  fno = functionName.as_object();
  success(fno->isBytes());
  functionLength = fno->lengthOf();
  if (!successFlag) return;
}



void Squeak_Interpreter::lookup_in_obsoleteNamedPrimitiveTable(Oop functionName, Object_p& fno, int functionLength,
                                                               Oop  moduleName, Object_p& mno, int moduleLength,
                                                               bool& on_main, fn_t& addr) { 
  // we use the obs named table to direct RVMPlugin prims to local core
  oop_int_t ii = obsoleteNamedPrimitiveTable.find( fno->as_char_p() + Object::BaseHeaderSize, functionLength, 
                                                   mno->as_char_p() + Object::BaseHeaderSize, moduleLength);
  if (!Abstract_Primitive_Table::is_index_valid(ii))
    return;
  
  on_main = obsoleteNamedPrimitiveTable.contents[ii].on_main;
  
  addr = The_Interactions.load_function_from_plugin(
                                                    on_main ? Logical_Core::main_rank : Logical_Core::my_rank(),
                                                    (char*)obsoleteNamedPrimitiveTable.contents[ii].newName,
                                                    (char*)obsoleteNamedPrimitiveTable.contents[ii].plugin);
  // recover from GC
  fno = functionName.as_object();
  mno = moduleName.as_object();
  
}


fn_t Squeak_Interpreter::munge_arguments_and_load_function_from_plugin_on_main(Oop functionName, Object_p& fno, int functionLength,
                                                                               Oop  moduleName, Object_p& mno, int moduleLength) { 
  char fn[10000], mod[10000];
  assert_always((size_t)functionLength < sizeof(fn)  &&  (size_t)moduleLength < sizeof(mod));
  strncpy( fn, fno->as_char_p() + Object::BaseHeaderSize, functionLength);  fn[functionLength] = '\0';
  strncpy(mod, mno->as_char_p() + Object::BaseHeaderSize,   moduleLength); mod[  moduleLength] = '\0';
  fn_t addr = The_Interactions.load_function_from_plugin(Logical_Core::main_rank, fn, mod);
  // recover from GC
  fno = functionName.as_object();
  mno = moduleName.as_object();
  
  return addr;
}


void Squeak_Interpreter::update_cache_and_call_external_function(Object_p fno, oop_int_t ii, fn_t addr, bool on_main) {
  static const bool verbose = false;
  
  methodCache.rewrite(roots.messageSelector, roots.lkupClass, 1000 + ii, addr, on_main);
  
  if (verbose) {
    stdout_printer->lprintf("in primitiveExternalCall (%d) %s: ",
                            increment_global_sequence_number(),
                            on_main ? "on main" : "here");
    fno->print_bytes(stdout_printer); stdout_printer->nl();
  }
  last_external_call_fn[rank_on_threads_or_zero_on_processes()] = addr;
  dispatchFunctionPointer(addr, on_main);
  
  if (verbose)
    lprintf("returned from prim %s\n", successFlag ? "succeeded" : "failed");
}


void Squeak_Interpreter::update_cache_and_report_failure_to_load_external_function(Object_p mno, Object_p fno) {
  if (mno->equals_string("SecurityPlugin")
      || fno->equals_string("primitivePluginBrowserReady")
      || fno->equals_string("primSetCompositionWindowPosition"))
  {}
  else {
    dittoing_stdout_printer->printf("Could not load external: ");
    fno->print(dittoing_stdout_printer); dittoing_stdout_printer->printf(" in ");
    mno->print(dittoing_stdout_printer); dittoing_stdout_printer->nl();
  }
  
  methodCache.rewrite(roots.messageSelector, roots.lkupClass, 0);
}





void Squeak_Interpreter::primitiveFindHandlerContext() {
  Oop thisCntx = popStack();
  Oop nil = roots.nilObj;
  for (;;) {
    if (!thisCntx.is_mem()  ||  thisCntx == nil) {
      push(nil);
      return;
    }
  Object_p tco = thisCntx.as_object();
  if (tco->isHandlerMarked()) {
      push(thisCntx);
      return;
    }
    thisCntx = tco->fetchPointer(Object_Indices::SenderIndex);
  }
}
void Squeak_Interpreter::primitiveFindNextUnwindContext() {
  Oop aContext = popStack();
  Oop nilOop = roots.nilObj;
  Object_p tco;

  for (Oop thisCntx  = popStack().as_object()->fetchPointer(Object_Indices::SenderIndex);
           thisCntx != aContext  &&  thisCntx != nilOop;
           thisCntx  = tco->fetchPointer(Object_Indices::SenderIndex)) {
    tco = thisCntx.as_object();
    if (tco->isUnwindMarked()) {
        push(thisCntx);
        return;
     }
  }
  push(nilOop);
}


void Squeak_Interpreter::primitiveFloatAdd() {
  primitiveFloatAdd(stackValue(1), stackTop());
}
void Squeak_Interpreter::primitiveFloatSubtract() {
  primitiveFloatSubtract(stackValue(1), stackTop());
}
void Squeak_Interpreter::primitiveFloatMultiply() {
  primitiveFloatMultiply(stackValue(1), stackTop());
}
void Squeak_Interpreter::primitiveFloatDivide() {
  primitiveFloatDivide(stackValue(1), stackTop());
}
void Squeak_Interpreter::primitiveFloatEqual() {
  bool b = primitiveFloatEqual(stackValue(1), stackTop());
  if (successFlag) { pop(2); pushBool(b); }
}
void Squeak_Interpreter::primitiveFloatGreaterOrEqual() {
  bool b = primitiveFloatLess(stackValue(1), stackTop());
  if (successFlag) { pop(2); pushBool(!b); }
}

void Squeak_Interpreter::primitiveFloatGreaterThan() {
  bool b = primitiveFloatGreater(stackValue(1), stackTop());
  if (successFlag) { pop(2); pushBool(b); }
}
void Squeak_Interpreter::primitiveFloatLessOrEqual() {
  bool b = primitiveFloatGreater(stackValue(1), stackTop());
  if (successFlag) { pop(2); pushBool(!b); }
}
void Squeak_Interpreter::primitiveFloatLessThan() {
  bool b = primitiveFloatLess(stackValue(1), stackTop());
  if (successFlag) { pop(2); pushBool(b); }
}
void Squeak_Interpreter::primitiveFloatNotEqual() {
  bool b = primitiveFloatEqual(stackValue(1), stackTop());
  if (successFlag) { pop(2); pushBool(!b); }
}
void Squeak_Interpreter::primitiveFlushCache() {
  flushMethodCacheMessage_class().send_to_all_cores();
}
void Squeak_Interpreter::primitiveFlushCacheByMethod() {
  pushRemappableOop(stackTop()); // in case of gc while message in transit
  flushByMethodMessage_class(stackTop()).send_to_all_cores();
  popRemappableOop();
}
void Squeak_Interpreter::primitiveFlushCacheSelective() {
  pushRemappableOop(stackTop()); // in case of gc while message in transit
  flushSelectiveMessage_class(stackTop()).send_to_all_cores();
  popRemappableOop();
}
void Squeak_Interpreter::primitiveFlushExternalPrimitives() {
  untested();
  flushExternalPrimitives();
}
void Squeak_Interpreter::primitiveForceDisplayUpdate() {
  ioForceDisplayUpdate();
}

void Squeak_Interpreter::primitiveFormPrint() {
  untested();
  bool landscapeFlag = booleanValueOf(stackTop());
  double vScale = floatValueOf(stackValue(1));
  double hScale = floatValueOf(stackValue(2));
  Oop rcvr = stackValue(3);
  if (!rcvr.is_mem())
    success(false);
  Object_p ro = rcvr.as_object();
  if (!successFlag) return;
  success(ro->lengthOf() >= 4);
  if (!successFlag) return;
  Oop bitsArray = ro->fetchPointer(0);
  success(bitsArray.is_mem());
  if (!successFlag) return;
  Object_p bo = bitsArray.as_object();
  int w = ro->fetchInteger(1);
  int h = ro->fetchInteger(2);
  int d = ro->fetchInteger(3);
  int pixelsPerWord = 32 / d;
  int wordsPerLine = (w + pixelsPerWord - 1) / pixelsPerWord;
  success(bo->isWordsOrBytes());
  if (!successFlag) return;
  int bitsArraySize = bo->byteLength();
  success(bitsArraySize == wordsPerLine * h * (int)sizeof(int32));
  if (!successFlag) return;
  // assume not 64 bit words
  success(ioFormPrint(bo->as_int32_p() + Object::BaseHeaderSize/sizeof(int32), w, h, d, hScale, vScale, landscapeFlag));
  if (!successFlag) return;
  pop(3);
}

void Squeak_Interpreter::primitiveFractionalPart() {
  double rcvr = popFloat();
  if (!successFlag) { unPop(1); return; }
  double trunc;
  pushFloat(modf(rcvr, &trunc));
}

void Squeak_Interpreter::primitiveFullGC() {
  pop(1);
  The_Memory_System()->incrementalGC();
  The_Memory_System()->fullGC("primitiveFullGC");
  pushInteger(The_Memory_System()->bytesLeft(true));
}

void Squeak_Interpreter::primitiveGetAttribute() {
  /*
   fetch the system attribute with the given integer ID. The
   result is a string, which will be empty if the attribute is not
   defined.
   */
  int attr = stackIntegerValue(0);
  if (!successFlag) return;
  int sz = attributeSize(attr);
  if (!successFlag) return;
  Object_p s = classString()->instantiateClass(sz);
  getAttributeIntoLength(attr, s->as_char_p() + Object::BaseHeaderSize, sz);
  popThenPush(2, s->as_oop());
}

void Squeak_Interpreter::primitiveGetNextEvent() {
  assert_external();
  const bool print = false;
  int evtBuf[evtBuf_size];
  for (int i = 0;  i < evtBuf_size;  ++i)  evtBuf[i] = 0;
  bool got_one = The_Interactions.getNextEvent_on_main(evtBuf); // do this from here so we can GC if need be
  if (!got_one) {
    primitiveFail();
    return;
  }
  
  successFlag = true;

  Oop arg = stackTop(); Object_p ao;
  if (!arg.is_mem() || !(ao = arg.as_object())->isArray() || ao->slotSize() != 8) {
    primitiveFail(); return;
  }
  int eventTypeIs = evtBuf[0];
  ao->storeInteger(0, evtBuf[0]);
  
  if (eventTypeIs == event_type_complex()) {
    for (int i = 1;  i < evtBuf_size;  ++i) {
      Oop v = Oop::from_bits(evtBuf[i]);
      if (check_assertions) v.okayOop();
      ao->storePointer(i, v);
    }
  }
  else {
    ao->storeInteger(1, evtBuf[1] & MillisecondClockMask);
    if (!successFlag) return;

    for (u_int32 i = 2;  i < sizeof(evtBuf)/sizeof(evtBuf[0]);  ++i) {
      oop_int_t v = evtBuf[i];
      if (Oop::isIntegerValue(v))
        ao->storeInteger(i, v);
      else {
        // may GC
        pushRemappableOop(arg);
        Oop value = Object::positive32BitIntegerFor(v);
        arg = popRemappableOop();
        ao = arg.as_object();
        ao->storePointer(i, value);
      }
    }
  }
  if (!successFlag) return;

  if (print && evtBuf[0] == 2)
    lprintf("primitiveGetNextEvent key: %d, state %d\n", evtBuf[2], evtBuf[3]);
  pop(1);
}

void Squeak_Interpreter::primitiveGreaterOrEqual() {
  oop_int_t a = popInteger();
  oop_int_t r = popInteger();
  checkBooleanResult(r >= a);
}
void Squeak_Interpreter::primitiveGreaterThan() {
  oop_int_t a = popInteger();
  oop_int_t r = popInteger();
  checkBooleanResult(r > a);
}
void Squeak_Interpreter::primitiveImageName() {
  /*
   When called with a single string argument, record the string as the current
   image file name. When called with zero arguments, return a string
   containing the current image file name.
   */
  if (get_argumentCount() == 1) {
    Oop s = stackTop();
    assertClass(s, splObj(Special_Indices::ClassString));
    if (!successFlag) return;
    Squeak_Image_Reader::imageNamePut_on_all_cores(s.as_object()->as_char_p() + Object::BaseHeaderSize, s.as_object()->stSize());
    pop(1);
  }
  else {
    oop_int_t sz = The_Memory_System()->imageNameSize();
    Object_p so = classString()->instantiateClass(sz);
    The_Memory_System()->imageNameGet(so, sz);
    pop(1);
    push(so->as_oop());
  }
}

void Squeak_Interpreter::primitiveIncrementalGC() {
  pop(1);
  The_Memory_System()->finalize_weak_arrays_since_we_dont_do_incrementalGC();
  pushInteger(The_Memory_System()->bytesLeft(false));
}

void Squeak_Interpreter::primitiveInputSemaphore() {
  /*
   "Register the input semaphore. The argument is an index into the
    ExternalObjectsArray part of the specialObjectsArray and must have been
    allocated via 'Smalltalk registerExternalObject: the Semaphore
   */
  Oop arg = stackTop();
  if (arg.is_int()) {
    // "If arg is integer, then condsider it as an index  into the external
    //  objects array and install it  as the new event semaphore
    ioSetInputSemaphore(arg.integerValue());
    if (successFlag) pop(1);
  }
}

void Squeak_Interpreter::primitiveInputWord() {
  // "Return an integer indicating the reason for the most recent input interrupt."
	popThenPushInteger(1, 0);	// "noop for now",
}

void Squeak_Interpreter::primitiveInstVarAt() {
  oop_int_t index = stackIntegerValue(0);
  Oop rcvr = stackValue(1);
  if (!rcvr.is_mem()) { primitiveFail(); return; }
  Object_p ro = rcvr.as_object();
  if (successFlag) {
    oop_int_t fixedFields = ro->fixedFieldsOfArray();
    if (index < 1  ||  index > fixedFields)
      successFlag = false;
  }
  if (successFlag) {
    Oop value = subscript(ro, index);
    if (successFlag) popThenPush(get_argumentCount() + 1, value);
  }
}
void Squeak_Interpreter::primitiveInstVarAtPut() {
  Oop newValue = stackTop();
  u_oop_int_t index = stackIntegerValue(1);
  Oop rcvr = stackValue(2);
  if (!rcvr.is_mem()) primitiveFail();
  if (!successFlag) return;
  Object_p ro = rcvr.as_object();
  if (index < 1  ||  index > ro->fixedFieldsOfArray()) {
    primitiveFail();
    return;
  }
  subscript(ro, index, newValue);
  if (successFlag) popThenPush(get_argumentCount() + 1, newValue);
}
void Squeak_Interpreter::primitiveInstVarsPutFromStack() {
  untested();
  unimplemented(); // obsolete
}
void Squeak_Interpreter::primitiveIntegerAt() {
  untested();
  unimplemented();
  oop_int_t index = stackIntegerValue(0);
  Oop rcvr = stackValue(1);
  Object_p ro;
  if (!rcvr.is_mem()  ||  !(ro = rcvr.as_object())->isWords()
      || index < 1  || index > (oop_int_t)ro->lengthOf() ) {
    success(false); return;
  }
  int32 value = ro->as_int32_p()[Object::BaseHeaderSize/sizeof(int32) + index - 1];
  pop(2);
  if (Oop::isIntegerValue(value))
    pushInteger(value);
  else
    push(Object::signed32BitIntegerFor(value));
}


void Squeak_Interpreter::primitiveIntegerAtPut() {
  untested();
  Oop valueOop = stackValue(0);
  oop_int_t index = stackIntegerValue(1);
  Oop rcvr = stackValue(2); Object_p ro;

  if (!rcvr.is_mem() || !(ro = rcvr.as_object())->isWords()
    ||  index < 1
    ||  index > (oop_int_t)ro->lengthOf() ) {
    success(false);
    return;
  }

  int32 value = valueOop.is_int() ? valueOop.integerValue() : signed32BitValueOf(valueOop);
  if (!successFlag) return;

  int32* addr = ro->as_int32_p() + Object::BaseHeaderSize/sizeof(int32) + index-1;
  The_Memory_System()->store_enforcing_coherence(addr, value, ro);
  pop(3);
  push(valueOop);
}


void Squeak_Interpreter::primitiveInterruptSemaphore() {
  Oop arg = popStack();
  roots.specialObjectsOop.as_object()->storePointer(Special_Indices::TheInterruptSemaphore,
                                                    arg.fetchClass() == splObj(Special_Indices::ClassSemaphore) ? arg : roots.nilObj);
}

void Squeak_Interpreter::primitiveInvokeObjectAsMethod() {
  if (check_many_assertions) assert(roots.newMethod.verify_oop());
  Object_p rao = splObj_obj(Special_Indices::ClassArray)->instantiateClass(get_argumentCount());
  rao->beRootIfOld();
  oopcpy_no_store_check(rao->as_oop_p() + Object::BaseHeaderSize/sizeof(Oop),
                        stackPointer() - (get_argumentCount() - 1),
                        get_argumentCount(),
                        rao);

  Oop runSelector = roots.messageSelector;
  Oop runReceiver = stackValue(get_argumentCount());
  pop(get_argumentCount() + 1);

  Oop newReceiver = roots.newMethod;
  assert(newReceiver.verify_oop());
  roots.messageSelector = splObj(Special_Indices::SelectorRunWithIn);
  set_argumentCount(3);

  push(newReceiver);
  push(runSelector);
  push(rao->as_oop());
  push(runReceiver);

  Oop lookupClass = newReceiver.fetchClass();
  assert(lookupClass.verify_oop());
  findNewMethodInClass(lookupClass);
  executeNewMethodFromCache();
  successFlag = true;
}

void Squeak_Interpreter::primitiveKbdNext() {
  pop(1);
  int keystrokeWord = ioGetKeystroke();
  if (keystrokeWord >= 0)  pushInteger(keystrokeWord);
  else                     push(roots.nilObj);

}

void Squeak_Interpreter::primitiveKbdPeek() {
  pop(1);
# if On_iOS
  int keystrokeWord = -1;
# else
  int keystrokeWord = ioPeekKeystroke();
# endif
  if (keystrokeWord >= 0)  pushInteger(keystrokeWord);
  else                     push(roots.nilObj);
}

void Squeak_Interpreter::primitiveLessOrEqual() {
  oop_int_t a = popInteger();
  oop_int_t r = popInteger();
  checkBooleanResult(r <= a);
}
void Squeak_Interpreter::primitiveLessThan() {
  oop_int_t a = popInteger();
  oop_int_t r = popInteger();
  checkBooleanResult(r < a);
}
void Squeak_Interpreter::primitiveListBuiltinModule() {
  untested();
  if (methodArgumentCount() != 1)  {
  primitiveFail(); return;}
  oop_int_t index = stackIntegerValue(0);
  if (index <= 0) { primitiveFail(); return; }
  char* moduleName = ioListBuiltinModule(index);
  if (moduleName == NULL) {
    pop(2);
    push(roots.nilObj);
    return;
  }
  Object_p nameObj = Object::makeString(moduleName);
  popThenPush(2, nameObj->as_oop());
  forceInterruptCheck();
}

void Squeak_Interpreter::primitiveListExternalModule() {
  untested();
  if (methodArgumentCount() != 1)  {
  primitiveFail(); return;}
  oop_int_t index = stackIntegerValue(0);
  if (index <= 0) { primitiveFail(); return; }
  char* moduleName = ioListExternalModule(index);
  if (moduleName == NULL) {
    pop(2);
    push(roots.nilObj);
    return;
  }
  Object_p nameObj = Object::makeString(moduleName);
  popThenPush(2, nameObj->as_oop());
  forceInterruptCheck();
}

void Squeak_Interpreter::primitiveLoadImageSegment() {
  untested();
  /*
   "This primitive is called from Squeak as...
   <imageSegment> loadSegmentFrom: aWordArray outPointers: anArray."

   "This primitive will load a binary image segment created by
    primitiveStoreImageSegment.  It expects the outPointer array to be of the
    proper size, and the wordArray to be well formed.  It will return as its
    value the original array of roots, and the erstwhile segmentWordArray will
    have been truncated to a size of zero.  If this primitive should fail, the
    segmentWordArray will, sadly, have been reduced to an unrecognizable and
    unusable jumble.  But what more could you have done with it anyway?
   */
  /*
  if (check_assertions) verifyCleanHeaders();
  Oop outPointerArray = stackTop();
  if (!outPointerArray.is_mem())  { primitiveFail();  return; }
  Object_p opao = outPointerArray.as_object();
  Oop* lastOut = opao.as_oop_p()  +  opao->lastPointer() / sizeof(Oop);
  Oop segmentWordArray = stackValue(1);
  if (!segmentWordArray.is_mem()) { primitiveFail();  return }
  Object_p sqao = segmentWordArray.as_object();
  Oop* endSeg = swao->as_oop_p() + (swao->sizeBits() - Object::BaseHeaderSize) / sizeof(Oop);

  if (apao->format() != Format::indexable_fields_only  ||  swao->format() != Format:indexable_word_fields_only) {
    primitiveFail();  return;
  }
  int32 data = swao->as_int32_p()[Object::BaseHeaderSize/sizeof(int32)];
  if (!Squeak_Image_Reader::readableFormat(data & 0xffff)) {
    reverseBytes(swao->as_char_p() + Object::BaseHeaderSize, endSeg + Object::BytesPerWord/sizeof(Oop));
  }
   */
  unimplemented(); // too much work
}
void Squeak_Interpreter::primitiveLoadInstVar() {
  Oop r = popStack();
  if (!r.is_mem()) {unPop(1); primitiveFail(); return; }
  push(r.as_object()->fetchPointer(primitiveIndex - 264));
}
void Squeak_Interpreter::primitiveLogN() {
  double r = popFloat();
  if (!successFlag) {unPop(1); return; }
  pushFloat(log(r));
}
void Squeak_Interpreter::primitiveLowSpaceSemaphore() {
  Oop arg = popStack();
  roots.specialObjectsOop.as_object()->storePointer(Special_Indices::TheLowSpaceSemaphore,
    arg.fetchClass() == splObj(Special_Indices::ClassSemaphore)  ?  arg  :  roots.nilObj);
}
void Squeak_Interpreter::primitiveMakePoint() {
  Oop a = stackTop();
  Oop r = stackValue(1);
  Object_p pto;
  if (r.is_int()) {
    if (a.is_int())
      pto = Object::makePoint(r.integerValue(), a.integerValue());
    else {
      pto = Object::makePoint(r.integerValue(), 0);
      pto->storePointer(1, stackTop()/*may have been changed by GC*/);
    }
  }
  else {
    Object_p ro = r.as_object();
    if (!ro->isFloatObject()) {
      successFlag = false;
      return;
    }
    pto = Object::makePoint(0,0);
    pto->storePointer(0, stackValue(1));
    pto->storePointer(1, stackValue(0));
  }
  popThenPush(2, pto->as_oop());
}
void Squeak_Interpreter::primitiveMarkHandlerMethod() {
  // Primitive. Mark the method for exception handling. The primitive must
  // fail after marking the context so that the regular code is run
  primitiveFail();
}
void Squeak_Interpreter::primitiveMarkUnwindMethod() {
  // "Primitive. Mark the method for exception unwinding.
  // The primitive must fail after marking the context so that the regular code is run();
  primitiveFail();
}


int Squeak_Interpreter::ioCPUMSecs() {
  struct rusage ru;
  getrusage(RUSAGE_SELF, &ru);
  return (ru.ru_utime.tv_sec + ru.ru_stime.tv_sec) * 1000  +  (ru.ru_utime.tv_usec + ru.ru_stime.tv_usec) / 1000;
}



void Squeak_Interpreter::primitiveMillisecondClock() {
  /*
   Return the value of the millisecond clock as an integer. Note that the
   millisecond clock wraps around periodically. On some platforms it can wrap
   daily. The range is limited to SmallInteger maxVal / 2 to allow delays of
   up to that length without overflowing a SmallInteger.
   */
  popThenPush(1, Oop::from_int(ioWhicheverMSecs() & MillisecondClockMask));
}
void Squeak_Interpreter::primitiveMod() {
  pop2AndPushIntegerIfOK(doPrimitiveMod( stackValue(1), stackTop()));
}
void Squeak_Interpreter::primitiveMouseButtons() {
  pop(1);
  pushInteger(ioGetButtonState());
}
void Squeak_Interpreter::primitiveMousePoint() {
  // mostly obsolete
  pop(1);
  int32 pw = ioMousePoint();
  push(Object::makePoint( pw >> 16,  (int32)(int16)(pw & 0xffff))->as_oop());
}
void Squeak_Interpreter::primitiveMultiply() {
  oop_int_t ir = stackIntegerValue(1);
  oop_int_t ia = stackIntegerValue(0);
  if (!successFlag) return;
  oop_int_t r = ir * ia;
  if (ia == 0  ||  ir / ia == r)
    pop2AndPushIntegerIfOK(r);
  else
    primitiveFail();
}


void Squeak_Interpreter::primitiveNew() {
  Oop klass = stackTop();
  // may GC
  Logical_Core* c = coreWithSufficientSpaceToInstantiate(klass, 0);
  success(c != NULL);
  if (successFlag)
    push( popStack().as_object()->instantiateClass(0, c)->as_oop());
}

void Squeak_Interpreter::primitiveNewMethod() {
  Oop header = popStack();
  oop_int_t bytecodeCount = popInteger();
  success(header.is_int());
  if (!successFlag) { unPop(2); return; }
  Oop klass = popStack();
  oop_int_t size = (Object::literalCountOfHeader(header.bits()) + 1) * bytesPerWord + bytecodeCount;
  Object_p thisMethod = klass.as_object()->instantiateClass(size);
  thisMethod->storePointer(Object_Indices::HeaderIndex, header);
  oop_int_t lc = Object::literalCountOfHeader(header.bits());
  for (int i = 1;  i <= lc;  ++i)
    thisMethod->storePointer(i, roots.nilObj);
  push(thisMethod->as_oop());
}

void Squeak_Interpreter::primitiveNewWithArg() {
  u_int32 size = positive32BitValueOf(stackTop());
  Oop klass    = stackValue(1);
  Logical_Core* c = NULL;

  success(int32(size) >= 0);
  if (successFlag) {
    c = coreWithSufficientSpaceToInstantiate(klass, size);
    success(c != NULL);
    klass = stackValue(1); // GC
  }

  if (successFlag)
    popThenPush(2, klass.as_object()->instantiateClass(size, c)->as_oop());
}

void Squeak_Interpreter::primitiveNext() {
  /*
   "PrimitiveNext will succeed only if the stream's array is in the atCache.
   Otherwise failure will lead to proper message lookup of at: and
   subsequent installation in the cache if appropriate."
   */
  Oop stream = stackTop();
  if (!stream.is_mem()) { primitiveFail(); return; }
  Object_p so = stream.as_object();
  if (!so->isPointers()  ||  so->lengthOf() <  Object_Indices::StreamReadLimitIndex + 1) {
    primitiveFail();
    return;
  }
  Oop array = so->fetchPointer(Object_Indices::StreamArrayIndex);
  oop_int_t index = so->fetchInteger(Object_Indices::StreamIndexIndex);
  oop_int_t limit = so->fetchInteger(Object_Indices::StreamReadLimitIndex);
  At_Cache::Entry* e = atCache.get_entry(array, false);
  if (index >= limit  ||  !e->matches(array)) {
    primitiveFail();
    return;
  }
  ++index;
  Oop result = commonVariableAt(array, index, e, false);
  // above may GC
  if (successFlag) {
    stream = stackTop();
    stream.as_object()->storeInteger(Object_Indices::StreamIndexIndex, index);
    popThenPush(1, result);
  }
}



void Squeak_Interpreter::primitiveNextInstance() {
  Oop obj = stackTop();
  Oop inst = The_Memory_System()->nextInstanceAfter(obj);
  if (inst == roots.nilObj)
    primitiveFail();
  else
    popThenPush(get_argumentCount() + 1, inst);
}

void Squeak_Interpreter::primitiveNextObject() {
  Oop obj = stackTop();
  if (!obj.is_mem())
    popThenPush(get_argumentCount() + 1, Oop::from_int(0));
  else {
    Oop x = The_Memory_System()->nextObject(obj);
    popThenPush(get_argumentCount() + 1, x);
  }
}


void Squeak_Interpreter::primitiveNextPut() {
  // only succeed if in atPutCache
  Oop value = stackTop();
  Oop stream = stackValue(1);
  Object_p so;
  if (!stream.isPointers()  || (so = stream.as_object())->lengthOf() < Object_Indices::StreamReadLimitIndex + 1) {
    primitiveFail();
    return;
  }

  Oop array = so->fetchPointer(Object_Indices::StreamArrayIndex);
  oop_int_t index = so->fetchInteger(Object_Indices::StreamIndexIndex);
  oop_int_t limit = so->fetchInteger(Object_Indices::StreamWriteLimitIndex); // Squeak bug, was StreamReadLimitIndex
  At_Cache::Entry* e = atCache.get_entry(array, true);
  if (index >= limit  ||  !e->matches(array)) {
    primitiveFail();
    return;
  }

  //cool
  ++index;
  commonVariableAtPut(array, index, value, e);
  if (successFlag) {
    so->storeInteger(Object_Indices::StreamIndexIndex, index);
    popThenPush(2, value);
  }
}


void Squeak_Interpreter::primitiveNoop() {
  pop(get_argumentCount());
}
void Squeak_Interpreter::primitiveNotEqual() {
  checkBooleanResult(!compare31or32BitsEqual(popStack(), popStack()));
}
void Squeak_Interpreter::primitiveObjectAt() {
  oop_int_t index = popInteger();
  Oop rcvr = popStack();
  Object_p ro;
  // only defined for compiled methods
  success(index > 0  &&  rcvr.is_mem()  &&   index <= (ro = rcvr.as_object())->literalCount() + Object_Indices::LiteralStart);
  if (successFlag)
    push(ro->fetchPointer(index - 1));
  else
    unPop(2);
}

void Squeak_Interpreter::primitiveObjectAtPut() {
  Oop newV = popStack();
  oop_int_t index = popInteger();
  Oop rcvr = popStack();
  Object_p ro;
  success(index > 0  &&  rcvr.is_mem()  &&  index <= (ro = rcvr.as_object())->literalCount() + Object_Indices::LiteralStart);
  if (successFlag) {
    ro->storePointer(index - 1, newV);
    push(newV);
  }
  else
    unPop(3);
}

void Squeak_Interpreter::primitiveObjectPointsTo() {
  Oop thang = popStack();
  Oop rcvr = popStack();
  if (rcvr.is_int()) {
    pushBool(false);
    return;
  }
  Object_p ro = rcvr.as_object();
  oop_int_t lastField = ro->lastPointer() / sizeof(Oop);
  for( int i = Object::BaseHeaderSize / sizeof(Oop);  i <= lastField;  ++i)
    if (ro->as_oop_p()[i] == thang) {
      pushBool(true);
      return;
    }
  pushBool(false);
}


void Squeak_Interpreter::primitiveObsoleteIndexedPrimitive() {
  // "Primitive. Invoke an obsolete indexed primitive."
  Obsolete_Indexed_Primitive_Table::entry& e = obsoleteIndexedPrimitiveTable.contents[primitiveIndex];
  if ( e.functionAddress != NULL) {
    dispatchFunctionPointer(e.functionAddress, true);
    return;
  }
  if ( e.pluginName == NULL  &&  e.functionName == NULL ) {
    primitiveFail();
    return;
  }
  e.functionAddress = The_Interactions.load_function_from_plugin(Logical_Core::main_rank, e.functionName, e.pluginName);
  if (e.functionAddress == NULL) {
    primitiveFail();
    return;
  }
  dispatchFunctionPointer(e.functionAddress, true);
}

void Squeak_Interpreter::primitivePerform() {
  Oop performSelector = roots.messageSelector;
  Oop performMethod = roots.newMethod;
  roots.messageSelector = stackValue(get_argumentCount() - 1);
  Oop newReceiver = stackValue(get_argumentCount());

  const bool print_startUps = false; // useful for debugging
  if (print_startUps && roots.messageSelector.as_object()->equals_string("startUp:")) {
    stdout_printer->printf(" startUp: "); newReceiver.print(stdout_printer); stdout_printer->nl();
    roots.messageSelector.print(dittoing_stdout_printer), stdout_printer->nl();
  }

  // Note: following lookup may fail

  // slide args down over sel
  set_argumentCount(get_argumentCount() - 1);
  int selectorIndex = stackPointerIndex() - get_argumentCount();
  transferFromIndexOfObjectToIndexOfObject(get_argumentCount(),
                                           selectorIndex + 1,
                                           activeContext_obj(),
                                           selectorIndex,
                                           activeContext_obj());
  pop(1);
  Oop lookupClass = newReceiver.fetchClass();
  findNewMethodInClass(lookupClass);

  {
  Object_p nmo = newMethod_obj();
  if (nmo->isCompiledMethod())
    success(nmo->argumentCount() == get_argumentCount());
  }

  if (successFlag) {
    executeNewMethodFromCache();
    successFlag = true;
  }
  else {
    for (int i = 1;  i <= get_argumentCount();  ++i)
      activeContext_obj()->storePointer(get_argumentCount() - i + 1 + selectorIndex,
                                        activeContext_obj()->fetchPointer(get_argumentCount() - i + selectorIndex));
    unPop(1);
    activeContext_obj()->storePointer(selectorIndex, roots.messageSelector);
    set_argumentCount(get_argumentCount() + 1);
    roots.newMethod = performMethod;
    roots.messageSelector = performSelector;
  }
}


void Squeak_Interpreter::primitivePerformInSuperclass() {
  untested();
  Oop lookupClass = stackTop();
  Oop rcvr = stackValue(get_argumentCount());
  Object_p cco;

  for (Oop currentClass = rcvr.fetchClass();
        currentClass != roots.nilObj;
        currentClass  = cco->superclass()) {
    if (currentClass == lookupClass) {
      popStack();
      primitivePerformAt(lookupClass);
      if (!successFlag)  push(lookupClass);
      return;
    }
    cco = currentClass.as_object();
  }
  primitiveFail();
}


void Squeak_Interpreter::primitivePerformWithArgs() {
  primitivePerformAt(stackValue(get_argumentCount()).fetchClass());
}

void Squeak_Interpreter::primitivePushFalse() {
  popStack();
  push(roots.falseObj);
}
void Squeak_Interpreter::primitivePushMinusOne() {
  untested();
  popStack();
  push(Oop::from_int(-1));
}
void Squeak_Interpreter::primitivePushNil() {
  popStack();
  push(roots.nilObj);
}
void Squeak_Interpreter::primitivePushOne() {
  popStack();
  push(Oop::from_int(1));
}
void Squeak_Interpreter::primitivePushSelf() {
  push(popStack());
}
void Squeak_Interpreter::primitivePushTrue() {
  popStack();
  push(roots.trueObj);
}
void Squeak_Interpreter::primitivePushTwo() {
  untested();
  popStack();
  push(Oop::from_int(2));
}
void Squeak_Interpreter::primitivePushZero() {
  untested();
  popStack();
  push(Oop::from_int(0));
}
void Squeak_Interpreter::primitiveQuit() {
  OS_Interface::sim_end_tracing();
  OS_Interface::profiler_disable();
  The_Measurements.print();
  ioExit();
 }

void Squeak_Interpreter::primitiveQuo() {
  oop_int_t intR = stackIntegerValue(1);
  oop_int_t intA = stackIntegerValue(0);
  success(intA != 0);
  if (!successFlag) return;
  pop2AndPushIntegerIfOK(
    intR > 0  ?  (intA > 0  ?     intR / intA   :  -intR / -intA)
              :  (intA > 0  ?  -(-intR / intA)  :  -intR / -intA));
}


// Our VM does not want to run the idle process, since it wastes cycles and power on a multicore CPU
// We have made the necessary changes to keep event handling going without it.
// -- dmu 10/1/10
void Squeak_Interpreter::primitiveRelinquishProcessor() {
  static Oop last_wayward_idle_process = Oop::from_bits(0);
  Oop this_process = roots.running_process_or_nil;
  if (last_wayward_idle_process.bits() != this_process.bits())
      lprintf("Deactivating caller process of primitiveRelinquishProcessor; assuming it's the idle process\n");;
  last_wayward_idle_process = this_process;
  
  if (Logical_Core::num_cores > 1)
    this_process.as_object()->store_allowable_cores_of_process(0LL); // not runnable anywhere
  pop(1);
  yield("stopping idle process");
}


void Squeak_Interpreter::primitiveResume() {
  Oop proc = stackTop();
  success(get_argumentCount() == 0  &&  proc.fetchClass() == splObj(Special_Indices::ClassProcess) );
  if (successFlag)  resume(proc, "primitiveResume");
  addedScheduledProcessMessage_class().send_to_other_cores();
}

void Squeak_Interpreter::primitiveScanCharacters() {
  if (methodArgumentCount() != 6) { primitiveFail();  return; }

  oop_int_t kernDelta = stackIntegerValue(0);
  Oop stops = stackObjectValue(1);
  Object_p so = stops.as_object();
  if (!so->isArray()) { primitiveFail();  return; }
  if (so->slotSize() < 258)  { primitiveFail();  return; }
  oop_int_t scanRightX = stackIntegerValue(2);
  Oop sourceString = stackObjectValue(3);
  Object_p sso = sourceString.as_object();
  if (!sso->isBytes()) { primitiveFail();  return; }
  oop_int_t scanStopIndex = stackIntegerValue(4);
  oop_int_t scanStartIndex = stackIntegerValue(5);
  if (scanStartIndex <= 0  ||  scanStopIndex <= 0  ||  scanStopIndex > sso->byteSize()) {
    primitiveFail();  return;
  }


  Oop rcvr = stackObjectValue(6);
  Object_p ro = rcvr.as_object();
  if (!ro->isPointers() || ro->slotSize() < 4) {
    primitiveFail();  return;
  }
  oop_int_t scanDestX = ro->fetchInteger(0);
  oop_int_t scanLastIndex = ro->fetchInteger(1);
  Oop scanXTable = ro->fetchPointer(2);
  Oop scanMap = ro->fetchPointer(3);
  Object_p sxto;
  if (!scanXTable.is_mem()  ||  !(sxto = scanXTable.as_object())->isArray()) {
    primitiveFail();  return;
  }
  Object_p smo;
  if (!scanMap.is_mem()  ||  (smo = scanMap.as_object())->slotSize() != 256) {
    primitiveFail();  return;
  }
  if (!successFlag) return;
  oop_int_t maxGlyph = sxto->slotSize() - 2;

  Oop nil = roots.nilObj;
  for ( scanLastIndex = scanStartIndex;  scanLastIndex <= scanStopIndex;  ++scanLastIndex ) {
    char ascii = sso->fetchByte(scanLastIndex - 1);
    Oop stopReason = so->fetchPointer( u_char(ascii) );
    if  ( stopReason != nil ) {
      if (!Oop::isIntegerValue(scanDestX)) {
        primitiveFail();  return;
      }
      ro->storeInteger(0, scanDestX);
      ro->storeInteger(1, scanLastIndex);
      pop(7);
      push(stopReason);
      return;
    }
     oop_int_t glyphIndex = smo->fetchInteger(u_char(ascii));
    if (failed() ||  glyphIndex > maxGlyph) {
      primitiveFail();  return;
    }
    oop_int_t sourceX = sxto->fetchInteger(glyphIndex);
    oop_int_t sourceX2 = sxto->fetchInteger(glyphIndex + 1);
    if (failed()) { return; }
    oop_int_t nextDestX = scanDestX + sourceX2 - sourceX;
    if (nextDestX > scanRightX) {
      if (!Oop::isIntegerValue(scanDestX))  {
        primitiveFail(); return;
      }
      ro->storeInteger(0, scanDestX);
      ro->storeInteger(1, scanLastIndex);
      pop(7);
      push(so->fetchPointer(Object_Indices::CrossedX - 1));
      return;
    }
    scanDestX = nextDestX + kernDelta;
  }
  if (!Oop::isIntegerValue(scanDestX)) {
    primitiveFail();  return;
  }
  ro->storeInteger(0, scanDestX);
  ro->storeInteger(1, scanStopIndex);
  pop(7);
  push(so->fetchPointer(Object_Indices::EndOfRun - 1));
}

void Squeak_Interpreter::primitiveScreenSize() {
  pop(1);
  u_int32 pw = ioScreenSize();
  push( Object::makePoint( pw >> 16,  pw & 0xffff)->as_oop());
}

void Squeak_Interpreter::primitiveSecondsClock() {
  popThenPush(1, Object::positive32BitIntegerFor(ioSeconds()));
}

void Squeak_Interpreter::primitiveSetDisplayMode() {
  untested();
  bool fsFlag = booleanValueOf(stackTop());
  int h = stackIntegerValue(1);
  int w = stackIntegerValue(2);
  int d = stackIntegerValue(3);
  if (successFlag) {
    bool okay = ioSetDisplayMode(w, h, d, fsFlag);
    if (successFlag) {
      pop(5);
      pushBool(okay);
    }
  }
}

void Squeak_Interpreter::primitiveSetFullScreen() {
  untested();
  Oop a = stackTop();
  if (a == roots.trueObj)
    ioSetFullScreen(true);
  else if (a == roots.falseObj)
    ioSetFullScreen(false);
  else
    primitiveFail();
  if (successFlag) pop(1);
}

void Squeak_Interpreter::primitiveSetGCSemaphore() {
  oop_int_t index = stackIntegerValue(0);
  if (successFlag) {
    set_gcSemaphoreIndex(index);
    pop(get_argumentCount());
  }
}

void Squeak_Interpreter::primitiveSetInterruptKey() {
  oop_int_t keycode = popInteger();
  if (successFlag)  set_interruptKeycode(keycode);
  else              unPop(1);
}

void Squeak_Interpreter::primitiveShortAt() {
  oop_int_t index = stackIntegerValue(0);
  Oop rcvr = stackValue(1); Object_p ro;
  success(rcvr.is_mem()  &&  (ro = rcvr.as_object())->isWordsOrBytes());
  if (!successFlag) return;
  oop_int_t sz = ro->sizeBits() - Object::BaseHeaderSize/sizeof(int16);
  success( index >= 1  &&  index <= sz );
  if (!successFlag) return;
  popThenPushInteger(2, ((int16*)(ro->as_char_p() + Object::BaseHeaderSize)) [index - 1]);
}

void Squeak_Interpreter::primitiveShortAtPut() {
  oop_int_t value = stackIntegerValue(0);
  oop_int_t index = stackIntegerValue(1);
  Oop rcvr = stackValue(2);  Object_p ro;
  success(rcvr.is_mem()  &&  (ro = rcvr.as_object())->isWordsOrBytes());
  if (!successFlag) return;
  oop_int_t sz = ro->sizeBits() - Object::BaseHeaderSize/sizeof(int16);
  success( index >= 1  &&  index <= sz );
  success( -32768 <= value  &&  value < 32768 );
  if (!successFlag) return;
  The_Memory_System()->store_enforcing_coherence(&((int16*)(ro->as_char_p() + Object::BaseHeaderSize))[index - 1], value, ro);
  pop(2);
}

void Squeak_Interpreter::primitiveShowDisplayRect() {
  displayBitsOf(
      displayObject(),
      stackIntegerValue(3), stackIntegerValue(1), stackIntegerValue(2), stackIntegerValue(0));
  if (successFlag) {
    ioForceDisplayUpdate();
    pop(4);
  }
}

void Squeak_Interpreter::primitiveSignal() {
  Oop sema = stackTop();
  assertClass(sema, splObj(Special_Indices::ClassSemaphore));
  if (successFlag) {
    Safepoint_Ability sa(false);
    sema.as_object()->synchronousSignal("primitiveSignal");
  }
}

void Squeak_Interpreter::primitiveSignalAtBytesLeft() {
  oop_int_t bytes = popInteger();
  The_Memory_System()->set_lowSpaceThreshold(successFlag ? bytes : 0);
  if (!successFlag)
    unPop(1);
}


void Squeak_Interpreter::primitiveSignalAtMilliseconds() {
  oop_int_t tick = popInteger();
  Oop sema = popStack();
  if (!successFlag) {
    unPop(2);
    return;
  }
  bool b = sema.fetchClass() == splObj(Special_Indices::ClassSemaphore);
  roots.specialObjectsOop.as_object()->
    storePointer(Special_Indices::TheTimerSemaphore,
                 b  ?  sema : roots.nilObj);
  set_nextWakeupTick(b ? tick : 0);
  // lprintf("set nextwakeuptick tick %d, now %d\n", tick, ioWhicheverMSecs() & MillisecondClockMask);
}


void Squeak_Interpreter::primitiveSine() {
  double r = popFloat();
  if (successFlag)
    pushFloat(sin(r));
  else
    unPop(1);
}


void Squeak_Interpreter::primitiveSize() {
  Oop rcvr = stackTop();
  if (rcvr.is_int()) {
    primitiveFail();
    return;
  }
  Object_p ro = rcvr.as_object();
  if ( Object::Format::has_only_fixed_fields(ro->format())) {
    primitiveFail();
    return;
  }
  int sz = ro->stSize();
  if (successFlag)
    popThenPush(1, Object::positive32BitIntegerFor(sz));
}


void Squeak_Interpreter::primitiveSnapshot() {
  snapshot(false);
}
void Squeak_Interpreter::primitiveSnapshotEmbedded() {
  untested();
  snapshot(true);
}
void Squeak_Interpreter::primitiveSomeInstance() {
  Oop klass = stackTop();
  Oop instance = The_Memory_System()->initialInstanceOf(klass);
  if (instance == roots.nilObj)
    primitiveFail();
  else
    popThenPush(get_argumentCount() + 1,  instance);
}

void Squeak_Interpreter::primitiveSomeObject() {
  pop(get_argumentCount() + 1);
  Oop r = The_Memory_System()->firstAccessibleObject();
  push(r);
}

void Squeak_Interpreter::primitiveSpecialObjectsOop() {
  popThenPush(1, roots.specialObjectsOop);
}

void Squeak_Interpreter::primitiveSquareRoot() {
  double rcvr = popFloat();
  success(rcvr >= 0.0);
  if (successFlag)  pushFloat(sqrt(rcvr));
  else               unPop(1);
}

void Squeak_Interpreter::primitiveStoreImageSegment() {
  untested();
  unimplemented();
}
void Squeak_Interpreter::primitiveStoreStackp() {
  Oop ctxt = stackValue(1);
  oop_int_t newStackp = stackIntegerValue(0);
  success(newStackp >= 0
      &&  newStackp <=  (Object_Indices::LargeContextSize - Object::BaseHeaderSize) / bytesPerWord  - Object_Indices::CtextTempFrameStart
      &&  ctxt.is_mem());
  if (!successFlag) { primitiveFail(); return; }
  Object_p co = ctxt.as_object();
  int stackp = co->fetchStackPointer();
  for (int i = stackp + 1;  i <= newStackp;  ++i)
    co->storePointer(i + Object_Indices::CtextTempFrameStart - 1,  roots.nilObj);
  co->storeStackPointerValue(newStackp);
  pop(1);
}

void Squeak_Interpreter::primitiveStringAt() {
  commonAt(true);
}
void Squeak_Interpreter::primitiveStringAtPut() {
  commonAtPut(true);
}

void Squeak_Interpreter::primitiveStringReplace() {
  Oop array = stackValue(4);
  oop_int_t start = stackIntegerValue(3);
  oop_int_t stop = stackIntegerValue(2);
  Oop repl = stackValue(1);
  oop_int_t replStart = stackIntegerValue(0);

  if (!successFlag ||  !array.is_mem()  ||  !repl.is_mem()) {
    primitiveFail();
    return;
  }
  Object_p ao = array.as_object();
  oop_int_t totalLength = ao->lengthOf();
  oop_int_t arrayInstSize = ao->fixedFieldsOfArray();
  if (start < 1  ||  start - 1 > stop  ||  stop + arrayInstSize > totalLength) {
    primitiveFail();
    return;
  }
  Object_p ro = repl.as_object();
  totalLength = ro->lengthOf();
  oop_int_t replInstSize = ro->fixedFieldsOfArray();
  if (replStart < 1  &&  stop - start + replStart + replInstSize > totalLength) {
    primitiveFail();
    return;
  }
  int arrayFmt = ao->format(),  replFmt = ro->format();
  bool formats_match =
    Object::Format::has_bytes(arrayFmt)
      ? (arrayFmt & ~Object::Format::byte_size_bits) == (replFmt & ~Object::Format::byte_size_bits)
      : arrayFmt == replFmt;
  if (!formats_match) { primitiveFail(); return; }

  oop_int_t srcIndex = replStart + replInstSize - 1;
  if (Object::Format::has_only_oops(arrayFmt))
    for (int i = start + arrayInstSize - 1;  i <= stop + arrayInstSize - 1;  ++i)
      ao->storePointer(i, ro->fetchPointer(srcIndex++));
  else if (!Object::Format::has_bytes(arrayFmt))
    for (int i = start + arrayInstSize - 1;  i <= stop + arrayInstSize - 1;  ++i)
      ao->storeLong32(i, ro->fetchLong32(srcIndex++));
  else
    for (int i = start + arrayInstSize - 1;  i <= stop + arrayInstSize - 1;  ++i)
      ao->storeByte( i, ro->fetchByte(srcIndex++));

  pop(get_argumentCount());
}


void Squeak_Interpreter::primitiveSubtract() {
  pop2AndPushIntegerIfOK(stackIntegerValue(1) - stackIntegerValue(0));
}

void Squeak_Interpreter::primitiveSuspend() {
  if (!primitiveThisProcess_was_called()) {
    static int kvetch_count = 10;
    if (kvetch_count) {
      lprintf("WARNING: primitiveSuspend called without primitiveThisProcess: this image probably cannot handle multithreading!\n");
      --kvetch_count;
      if (!kvetch_count) lprintf("WARNING: This is your last warning!\n");
    }
  }

  Oop procToSuspend = stackTop();
  if ( procToSuspend.fetchClass() != splObj(Special_Indices::ClassProcess) ) {
    primitiveFail();
    return;
  }
  Oop old_list;
  {
    Scheduler_Mutex sm("primitiveSuspend");
    Object_p proc = procToSuspend.as_object();
    old_list = proc->remove_process_from_scheduler_list("primitiveSuspend");
    pop(1);
    push(old_list);
    if (get_running_process() == procToSuspend)  {
      put_running_process_to_sleep("primitiveSuspend");
      transfer_to_highest_priority("primitiveSuspend");      
    }
    else set_yield_requested(true);
  }
}


static void terminate_to(Oop aContext, Oop thisCntx) {
  // Warning: only works if called for this very process
  Object_p aco = aContext.as_object();
  Object_p tco = thisCntx.as_object();
  
  if (tco->hasSender(aContext)) {
    Oop nilOop = The_Squeak_Interpreter()->roots.nilObj;
    Object_p nextCntx;
    for (Object_p currentCntx = tco->fetchPointer(Object_Indices::SenderIndex).as_object();
         currentCntx != aco;
         currentCntx = nextCntx) {
      nextCntx = currentCntx->fetchPointer(Object_Indices::SenderIndex).as_object();
      currentCntx->storePointer(Object_Indices::            SenderIndex, nilOop);
      currentCntx->storePointer(Object_Indices::InstructionPointerIndex, nilOop);
    }
  }
  tco->storePointer(Object_Indices::SenderIndex, aContext);
}


void Squeak_Interpreter::primitiveTerminateTo() {
  // Warning: only works if called for this very process
  Oop aContext = popStack();
  Oop thisCntx = popStack();

  if (!aContext.is_mem()  ||  !thisCntx.is_mem()) {
    unPop(2);
    primitiveFail();
    return;
  }
  terminate_to(aContext, thisCntx);
  push(thisCntx);
}


void Squeak_Interpreter::primitiveTestDisplayDepth() {
  oop_int_t bitsPerPixel = stackIntegerValue(0);
  if (!successFlag) return;
  pop(2);
  pushBool(ioHasDisplayDepth(bitsPerPixel));
}

void Squeak_Interpreter::primitiveTimesTwoPower() {
  oop_int_t arg = popInteger();
  double rcvr = popFloat();
  if (successFlag)
    pushFloat(ldexp(rcvr, arg));
  else
    unPop(2);
}

void Squeak_Interpreter::primitiveTruncated() {
  double rcvr = popFloat();
  double trunc;
  if (successFlag) {
    modf(rcvr, &trunc);
    // XXX ranges wrong if smallints not 31 bits xxx_dmu 64
    success(double(MinSmallInt) <= trunc  &&  trunc <= double(MaxSmallInt));
  }
  if (successFlag)  pushInteger(oop_int_t(trunc));
  else unPop(1);
}

void Squeak_Interpreter::primitiveUnloadModule() {
  untested();
  unimplemented();
}

void Squeak_Interpreter::primitiveVMParameter() {
  lprintf("primitiveVMParameter really not done\n");
  static const int paramsArraySize = 40;
  if (get_argumentCount() == 0) {
    Object_p ro = splObj_obj(Special_Indices::ClassArray)->instantiateClass(paramsArraySize);
    for (int i = 0;  i < paramsArraySize;  ++i)  ro->storePointer(i, Oop::from_int(0));
    ro->storePointer(23, Oop::from_int(The_Memory_System()->get_shrinkThreshold()));
    ro->storePointer(24, Oop::from_int(The_Memory_System()->get_growHeadroom()   ));

    popThenPush(1, ro->as_oop());
  }
  else if (get_argumentCount() == 1) {
    Oop arg = stackTop();
    if (!arg.is_int()) { primitiveFail(); return; }
    oop_int_t argi = arg.integerValue();
    oop_int_t result;
    switch (argi) {
        default:
          lprintf("primitiveVMParameter: attempt to get %d\n", argi);
          primitiveFail();
          return;
        case 24: result = The_Memory_System()->get_shrinkThreshold(); break;
        case 25: result = The_Memory_System()->get_growHeadroom(); break;
    }
    popThenPush(2, Oop::from_int(result));
  }
  else if (get_argumentCount() == 2) {
    Oop val = stackTop();
    Oop index = stackValue(1);
    oop_int_t result;
    if (!val.is_int() || !index.is_int()) {
      primitiveFail();
      return;
    }
    oop_int_t vi = val.integerValue();
    oop_int_t ii = index.integerValue();
    switch (ii) {
        default:
          static bool warned = false;  // Stefan: think, does not need to be threadsafe. 2009-09-05
          if (!warned) {
            warned = true;
            lprintf("vmParameter %d = %d unimplemented (suppresing additional warnings)\n", ii, vi);
          }
          result = 0;
          break;
        case 24: result = The_Memory_System()->get_shrinkThreshold();  The_Memory_System()->set_shrinkThreshold(vi);  break;
        case 25: result = The_Memory_System()->get_growHeadroom();     The_Memory_System()->set_growHeadroom(vi);     break;
    }
    popThenPush(3, Oop::from_int(result));
  }
  else primitiveFail();
}

void Squeak_Interpreter::primitiveVMPath() {
  oop_int_t sz = vmPathSize();
  Object_p s = classString()->instantiateClass(sz);
  vmPathGetLength(s->as_char_p() + Object::BaseHeaderSize, sz);
  popThenPush(1, s->as_oop());
}

void Squeak_Interpreter::primitiveValue() {
  Oop blockContext = stackValue(get_argumentCount());
  if (!blockContext.is_mem()) { primitiveFail(); return; }
  Object_p bco = blockContext.as_object();
  oop_int_t blockArgumentCount = bco->argumentCountOfBlock();
  success(get_argumentCount() == blockArgumentCount
      &&  bco->fetchPointer(Object_Indices::CallerIndex) == roots.nilObj);
  if (!successFlag) return;

  transferFromIndexOfObjectToIndexOfObject(
                                           get_argumentCount(),
                                           stackPointerIndex() - get_argumentCount() + 1,
                                           activeContext_obj(),
                                           Object_Indices::TempFrameStart,
                                           bco);
  // assume prev call made blockContext a root

  pop(get_argumentCount()+1);
  Oop initialIP = bco->fetchPointer(Object_Indices::InitialIPIndex);
  assert(initialIP.is_int());
  bco->storePointerUnchecked(Object_Indices::InstructionPointerIndex, initialIP);
  bco->storeStackPointerValue(get_argumentCount());
  bco->storePointerUnchecked(Object_Indices::CallerIndex, activeContext());
  newActiveContext(blockContext, bco);
}

void Squeak_Interpreter::primitiveValueUninterruptably() {
  untested();
  primitiveValue();
}
void Squeak_Interpreter::primitiveValueWithArgs() {
  Oop argumentArray = popStack();
  Oop blockContext = popStack();
  Object_p argumentArray_obj;
  Object_p blockContext_obj;

  if (!argumentArray.is_mem()  ||  !blockContext.is_mem()
      ||  !(argumentArray_obj = argumentArray.as_object())->isArray()) {
    unPop(2);
    primitiveFail();
    return;
  }
  blockContext_obj = blockContext.as_object();
  oop_int_t blockArgumentCount = blockContext_obj->argumentCountOfBlock(), arrayArgumentCount;

  if (successFlag) {
    arrayArgumentCount = argumentArray_obj->fetchWordLength();
    success( arrayArgumentCount == blockArgumentCount
         &&  blockContext_obj->fetchPointer(Object_Indices::CallerIndex) == roots.nilObj );
  }
  if (successFlag) {
    transferFromIndexOfObjectToIndexOfObject(arrayArgumentCount,
                                             0,
                                             argumentArray_obj,
                                             Object_Indices::TempFrameStart,
                                             blockContext_obj);

    Oop initialIP = blockContext_obj->fetchPointer(Object_Indices::InitialIPIndex);
    blockContext_obj->storePointerUnchecked(Object_Indices::InstructionPointerIndex, initialIP);
    blockContext_obj->storeStackPointerValue(arrayArgumentCount);
    blockContext_obj->storePointerUnchecked(Object_Indices::CallerIndex, activeContext());
    newActiveContext(blockContext, blockContext_obj);
  }
  else
    unPop(2);
}

# if Include_Closure_Support

void Squeak_Interpreter::primitiveClosureValue() {
  Oop blockClosure = stackValue(get_argumentCount());
  if (!blockClosure.is_mem()) { primitiveFail(); return; }
  Object_p blockClosure_obj = blockClosure.as_object();
  int blockArgumentCount = blockClosure_obj->argumentCountOfClosure();
  if ( !successFlag || get_argumentCount() != blockArgumentCount) { 
    primitiveFail(); 
    return; 
  }
  
  Oop outerContext = blockClosure_obj->fetchPointer(Object_Indices::ClosureOuterContextIndex);
  if (!outerContext.isContext()) { primitiveFail(); return; }
  Object_p outerContext_obj = outerContext.as_object();
  
  Oop closureMethod = outerContext_obj->fetchPointer(Object_Indices::MethodIndex);
  Object_p closureMethod_obj = closureMethod.as_object();
  if (!closureMethod_obj->isCompiledMethod()) { primitiveFail(); return; }
  
  activateNewClosureMethod(blockClosure_obj, (Object_p)NULL);
  if ( !The_Squeak_Interpreter()->doing_primitiveClosureValueNoContextSwitch)
    quickCheckForInterrupts();
}


/*
 What follows is an explanation of the following primitive from Eliot Miranda: (dmu 6/8/10)
 
  Eliot,

  I am currently bringing a ST VM up to 4.1, and saw that the image wants a primitive called:
  primitiveClosureValueNoContextSwitch
  What gives? Why is the NoContextSwitch property needed? Can you point me to an explanation?
 
 Eliot writes:

  It is to do with critical sections and unwinds, e.g. Semaphore>>critical: and BlockClosure>>ensure:.

  Take the following def of Semaphore>>critical:

  critical: mutuallyExcludedBlock
  "Evaluate mutuallyExcludedBlock only if the receiver is not currently in
  the process of running the critical: message. If the receiver is, evaluate
  mutuallyExcludedBlock after the other critical: message is finished."

  [self wait.
   aBlock value] ensure: [self signal]

  If there is a preemption point on block activation (natural since they build frames and so in Peter's VM style,
 testing for events on stack overflow, there is a preemption point on block activation) then the
 process can be preempted after entering  [self wait. aBlock value] but before evaluating
 self wait   and so if the process is unwound (e.g. via Process>>terminate issued form some other process)
 the semaphore can gain an extra signal.
 
 Squeak 4.1 doesn't yet use this knowledge in Semaphore>>critical: but I believe it can.
 
 -- Eliot Miranda 6/2010
*/                                                    


void Squeak_Interpreter::primitiveClosureValueNoContextSwitch() {
  The_Squeak_Interpreter()->doing_primitiveClosureValueNoContextSwitch = true;
  primitiveClosureValue(); 
}



void Squeak_Interpreter::primitiveClosureValueWithArgs() {
  Oop argumentArray = stackTop();
  if (!argumentArray.is_mem()) { primitiveFail(); return; }
  Object_p argumentArray_obj = argumentArray.as_object();
  
  // check for enough space in thisContext to push all args
  int arraySize = argumentArray_obj->fetchWordLength();
  int cntxSize = activeContext_obj()->fetchWordLength();
  if ( stackPointerIndex() + arraySize  >=  cntxSize) { primitiveFail(); return; }
  
  Oop blockClosure = stackValue(get_argumentCount());
  if (blockClosure.fetchClass() != splObj(Special_Indices::ClassBlockClosure)) { primitiveFail(); return; }
  Object_p blockClosure_obj = blockClosure.as_object();
  int blockArgumentCount = blockClosure_obj->argumentCountOfClosure();
  if ( arraySize != blockArgumentCount ) { primitiveFail(); return; }
  
  // paranoid check could discard later
  Oop outerContext = blockClosure_obj->fetchPointer(Object_Indices::ClosureOuterContextIndex);
  if (!outerContext.isContext()) { primitiveFail(); return; }
  Object_p outerContext_obj = outerContext.as_object();
  
  Oop closureMethod = outerContext_obj->fetchPointer(Object_Indices::MethodIndex);
  Object_p closureMethod_obj = closureMethod.as_object();
  if (!closureMethod_obj->isCompiledMethod()) { primitiveFail(); return; }
  
  popStack();
  
  // Copy args to stack and activate
  for (int i = 1;  i <= arraySize;  ++i )
    push( argumentArray_obj->fetchPointer(i - 1));
  
  set_argumentCount(arraySize);
  activateNewClosureMethod(blockClosure_obj, (Object_p)NULL);
  quickCheckForInterrupts();
}

# endif


void Squeak_Interpreter::primitiveWait() {
  Oop sema = stackTop();
  if (!sema.is_mem()) {  primitiveFail(); return; }
  Object_p so = sema.as_object();
  assertClass(so, splObj(Special_Indices::ClassSemaphore));
  Semaphore_Mutex sm("primitiveWait");
  if (successFlag) {
    oop_int_t excessSignals = so->fetchInteger(Object_Indices::ExcessSignalsIndex);
    if (excessSignals > 0)
      so->storeInteger(Object_Indices::ExcessSignalsIndex, excessSignals - 1);
    else {
      Scheduler_Mutex sm("primitiveWait");
      Oop activeProc = remove_running_process_from_scheduler_lists_and_put_it_to_sleep("primitiveWait");
      so->addLastLinkToList(activeProc);
      transfer_to_highest_priority("primitiveWait");
      if (Check_Prefetch) assert_always(have_executed_currentBytecode);
    }
  }
}

void Squeak_Interpreter::primitiveYield() {
  yield("primitiveYield");
}


void Squeak_Interpreter::startProfiling() {
  untested();
  unimplemented();
}
void Squeak_Interpreter::stopProfiling()  {
  untested();
  unimplemented();
}



void Squeak_Interpreter::clearProfile() {
  untested();
  unimplemented();
}
void Squeak_Interpreter::dumpProfile()  {
  untested();
  unimplemented();
}







void Squeak_Interpreter::primitiveFloatAdd(Oop r, Oop a) {
  double rd = loadFloatOrIntFrom(r);
  double ad = loadFloatOrIntFrom(a);
  if (successFlag) {
    pop(2);
    pushFloat(rd + ad);
  }
}

void Squeak_Interpreter::primitiveFloatSubtract(Oop r, Oop a) {
  double rd = loadFloatOrIntFrom(r);
  double ad = loadFloatOrIntFrom(a);
  if (successFlag) {
    pop(2);
    pushFloat(rd - ad);
  }
}

void Squeak_Interpreter::primitiveFloatMultiply(Oop r, Oop a) {
  double rd = loadFloatOrIntFrom(r);
  double ad = loadFloatOrIntFrom(a);
  if (successFlag) {
    pop(2);
    pushFloat(rd * ad);
  }
}

void Squeak_Interpreter::primitiveFloatDivide(Oop r, Oop a) {
  double rd = loadFloatOrIntFrom(r);
  double ad = loadFloatOrIntFrom(a);
  if (successFlag) {
    success(ad != 0.0);
    if (successFlag) {
      pop(2);
      pushFloat(rd / ad);
    }
  }
}

bool Squeak_Interpreter::primitiveFloatLess(Oop r, Oop a) {
  double rd = loadFloatOrIntFrom(r);
  double ad = loadFloatOrIntFrom(a);
  return  successFlag  ?  rd < ad  :  0.0;
}

bool Squeak_Interpreter::primitiveFloatGreater(Oop r, Oop a) {
  double rd = loadFloatOrIntFrom(r);
  double ad = loadFloatOrIntFrom(a);
  return  successFlag  ?  rd > ad  :  0.0;
}

bool Squeak_Interpreter::primitiveFloatEqual(Oop r, Oop a) {
  double rd = loadFloatOrIntFrom(r);
  double ad = loadFloatOrIntFrom(a);
  return  successFlag  ?  rd == ad  :  0.0;
}

# if Include_Closure_Support
void Squeak_Interpreter::primitiveClosureCopyWithCopiedValues() {
  Safepoint_Ability sa(false);
  int numArgs = stackIntegerValue(1);
  Oop copiedValues = stackTop();
  success( copiedValues.fetchClass() == splObj(Special_Indices::ClassArray) );
  if (!successFlag)  { return; }  
  
  Object_p copiedValues_obj = copiedValues.as_object();
  int numCopiedValues = copiedValues_obj->fetchWordLength();
  
  Oop newClosure = closureCopy(numArgs,
                               // greater by 1 because of preincre of localIP
                              instructionPointer() + 2 - (method_obj()->as_u_char_p() + Object::BaseHeaderSize),
                               numCopiedValues);
  Object_p newClosure_obj = newClosure.as_object();
  newClosure_obj->storePointer(Object_Indices::ClosureOuterContextIndex, stackValue(2));
  if (numCopiedValues > 0) {
    // alloc for gc may have moved copied values
    copiedValues = stackTop();
    copiedValues_obj = copiedValues.as_object();
    for ( int i = 0;  i < numCopiedValues;  ++i )
      newClosure_obj->storePointerUnchecked(i + Object_Indices::ClosureFirstCopiedValueIndex, copiedValues_obj->fetchPointer(i));
  }
  popThenPush(3, newClosure);
}

# endif
