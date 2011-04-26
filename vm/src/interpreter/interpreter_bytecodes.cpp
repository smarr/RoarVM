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

void Squeak_Interpreter::pushReceiverVariableBytecode() {
  fetchNextBytecode();
  pushReceiverVariable(prevBytecode & 0xf);
}
void Squeak_Interpreter::pushTemporaryVariableBytecode() {
  fetchNextBytecode();
  pushTemporaryVariable(prevBytecode & 0xf);
}
void Squeak_Interpreter::pushLiteralConstantBytecode() {
  fetchNextBytecode();
  pushLiteralConstant(prevBytecode & 0x1f);
}
void Squeak_Interpreter::pushLiteralVariableBytecode() {
  fetchNextBytecode();
  pushLiteralVariable(prevBytecode & 0x1f);
}

void Squeak_Interpreter::storeAndPopReceiverVariableBytecode() {
  fetchNextBytecode();
  // could watch for suspended context change here
  receiver_obj()->storePointer(prevBytecode & 7, internalStackTop());
  internalPop(1);
}

void Squeak_Interpreter::storeAndPopTemporaryVariableBytecode() {
  fetchNextBytecode();
  assert(_localHomeContext != roots.nilObj.as_object());
	localHomeContext()->storePointerIntoContext((prevBytecode & 7) + Object_Indices::TempFrameStart, internalStackTop());
	internalPop(1);

}
void Squeak_Interpreter::pushReceiverBytecode() {
  fetchNextBytecode();
  internalPush(roots.receiver);
}
void Squeak_Interpreter::pushConstantTrueBytecode() {
  fetchNextBytecode();
  internalPush(roots.trueObj);
}
void Squeak_Interpreter::pushConstantFalseBytecode() {
  fetchNextBytecode();
  internalPush(roots.falseObj);
}
void Squeak_Interpreter::pushConstantNilBytecode() {
  fetchNextBytecode();
  internalPush(roots.nilObj);
}
void Squeak_Interpreter::pushConstantMinusOneBytecode() {
  fetchNextBytecode();
  internalPush(Oop::from_int(-1));
}
void Squeak_Interpreter::pushConstantZeroBytecode() {
  fetchNextBytecode();
  internalPush(Oop::from_int(0));
}
void Squeak_Interpreter::pushConstantOneBytecode() {
  fetchNextBytecode();
  internalPush(Oop::from_int(1));
}
void Squeak_Interpreter::pushConstantTwoBytecode() {
  fetchNextBytecode();
  internalPush(Oop::from_int(2));
}

void Squeak_Interpreter::returnReceiver() {
  commonReturn(sender(), roots.receiver);
}
void Squeak_Interpreter::returnTrue() {
  commonReturn(sender(), roots.trueObj);
}
void Squeak_Interpreter::returnFalse() {
  commonReturn(sender(), roots.falseObj);
}
void Squeak_Interpreter::returnNil() {
  commonReturn(sender(), roots.nilObj);
}

void Squeak_Interpreter::returnTopFromMethod() {
  commonReturn(sender(), internalStackTop());
}
void Squeak_Interpreter::returnTopFromBlock() {
  commonReturn(caller(), internalStackTop());
}


void Squeak_Interpreter::unknownBytecode() {
  untested();
  fatal("unknown bytecode");
}

void Squeak_Interpreter::extendedPushBytecode() {
  u_char descriptor = fetchByte();
  fetchNextBytecode();
  int i = descriptor & 0x3f;
  switch ((descriptor >> 6) & 3) {
    case 0: pushReceiverVariable(i); break;
    case 1: pushTemporaryVariable(i); break;
    case 2: pushLiteralConstant(i); break;
    case 3: pushLiteralVariable(i); break;
  }
}

void Squeak_Interpreter::extendedStoreBytecode() {
  u_char d = fetchByte();
  fetchNextBytecode();
  u_char vi = d & 63;
  switch ((d >> 6) & 3) {
    case 0:
      // could watch for suspended context change here
      receiver_obj()->storePointer(vi, internalStackTop());
      break;
    case 1:
      localHomeContext()->storePointerIntoContext(
                                                vi + Object_Indices::TempFrameStart, internalStackTop());
      break;
    case 2:
      fatal("illegal store");
    case 3:
      literal(vi).as_object()->storePointer(Object_Indices::ValueIndex, internalStackTop());
      break;
  }
}

void Squeak_Interpreter::extendedStoreAndPopBytecode() {
  extendedStoreBytecode();
  internalPop(1);
}
void Squeak_Interpreter::singleExtendedSendBytecode() {
  u_char d = fetchByte();
  roots.messageSelector = literal(d & 0x1f);
  set_argumentCount( d >> 5 );
  normalSend();
}
void Squeak_Interpreter::doubleExtendedDoAnythingBytecode() {
  /*
   "Replaces the Blue Book double-extended send [132], in which the first byte 
    was wasted on 8 bits of argument count.
    Here we use 3 bits for the operation sub-type (opType),  and the remaining
    5 bits for argument count where needed.
    The last byte give access to 256 instVars or literals.
    See also secondExtendedSendBytecode"
   */
  u_char b2 = fetchByte();
  u_char b3 = fetchByte();
  switch (b2 >> 5) {
    case 0:
      roots.messageSelector = literal(b3);
      set_argumentCount( b2 & 31 );
      normalSend();
      break;
    case 1:
      roots.messageSelector = literal(b3);
      set_argumentCount( b2 & 31);
      superclassSend();
      break;
    case 2:
      fetchNextBytecode();
      pushReceiverVariable(b3);
      break;
    case 3:
      fetchNextBytecode();
      pushLiteralConstant(b3);
      break;
    case 4:
      fetchNextBytecode();
      pushLiteralVariable(b3);
      break;
    case 5:
      fetchNextBytecode();
      // could watch for suspended context change here
      receiver_obj()->storePointer(b3, internalStackTop());
      break;
    case 6: {
      fetchNextBytecode();
      Oop top = internalStackTop();
      internalPop(1);
      // could watch for suspended context change here
      receiver_obj()->storePointer(b3, top);
      break;
    }
    case 7:
      fetchNextBytecode();
      literal(b3).as_object()->storePointer(Object_Indices::ValueIndex, internalStackTop());
      break;
  }
}
void Squeak_Interpreter::singleExtendedSuperBytecode() {
  u_char d = fetchByte();
  roots.messageSelector = literal(d & 0x1f);
  set_argumentCount( d >> 5 );
  superclassSend();
}

void Squeak_Interpreter::secondExtendedSendBytecode() {
  /*
   This replaces the Blue Book double-extended super-send [134],
   which is subsumed by the new double-extended do-anything [132].
   It offers a 2-byte send of 0-3 args for up to 63 literals, for which
   the Blue Book opcode set requires a 3-byte instruction."
   */
  u_char descriptor = fetchByte();
  roots.messageSelector = literal(descriptor & 0x3f);
  set_argumentCount( descriptor >> 6 );
  assert (!internalStackValue(get_argumentCount()).is_mem()
          || The_Memory_System()->object_table->probably_contains((void*)internalStackValue(get_argumentCount()).bits()));
  normalSend();
}

void Squeak_Interpreter::popStackBytecode() { fetchNextBytecode(); internalPop(1); }

void Squeak_Interpreter::duplicateTopBytecode() {
  fetchNextBytecode();
  internalPush(internalStackTop());
}
void Squeak_Interpreter::pushActiveContextBytecode() {
  fetchNextBytecode();
  reclaimableContextCount = 0;
  internalPush(activeContext());
}
void Squeak_Interpreter::experimentalBytecode() {
  untested();
  unimplemented();
}
void Squeak_Interpreter::shortUnconditionalJump() { jump((currentBytecode & 7) + 1); }
void Squeak_Interpreter::shortConditionalJump()   { jumpIfFalseBy((currentBytecode & 7) + 1); }


void Squeak_Interpreter::longUnconditionalJump() {
  int offset = long_jump_offset();
  set_localIP(localIP() + offset);
  if (offset < 0)
    internalQuickCheckForInterrupts();
  fetchNextBytecode();
}
void Squeak_Interpreter::longJumpIfTrue() {
  jumpIfTrueBy(long_cond_jump_offset());
}
void Squeak_Interpreter::longJumpIfFalse() {
  jumpIfFalseBy(long_cond_jump_offset());
}

void Squeak_Interpreter::bytecodePrimAdd() {
  Oop rcvr = internalStackValue(1);
  Oop arg  = internalStackValue(0);
  if (areIntegers(rcvr, arg)) {
    oop_int_t r = rcvr.integerValue() + arg.integerValue();
    if (Oop::isIntegerValue(r)) {
      internalPopThenPush(2, Oop::from_int(r));
      fetchNextBytecode();
      return;
    }
  }
  else {
    successFlag = true;
    externalizeIPandSP();
    {
      Safepoint_Ability sa(true);
      primitiveFloatAdd(rcvr, arg);
    }
    internalizeIPandSP();
    if (successFlag) {
      fetchNextBytecode();
      return;
    }
  }
  roots.messageSelector = specialSelector(0);
  set_argumentCount(1);
  normalSend();
}

void Squeak_Interpreter::bytecodePrimSubtract() {
  Oop rcvr = internalStackValue(1);
  Oop arg  = internalStackValue(0);
  if (areIntegers(rcvr, arg)) {
    oop_int_t r = rcvr.integerValue() - arg.integerValue();
    if (Oop::isIntegerValue(r)) {
      internalPopThenPush(2, Oop::from_int(r));
      fetchNextBytecode();
      return;
    }
  }
  else {
    successFlag = true;
    externalizeIPandSP();
    {
      Safepoint_Ability sa(true);
      primitiveFloatSubtract(rcvr, arg);
    }
    internalizeIPandSP();
    if (successFlag) {
      fetchNextBytecode();
      return;
    }
  }
  roots.messageSelector = specialSelector(1);
  set_argumentCount(1);
  normalSend();
}

void Squeak_Interpreter::bytecodePrimMultiply() {
  Oop rcvr = internalStackValue(1);
  Oop arg  = internalStackValue(0);
  if (areIntegers(rcvr, arg)) {
    oop_int_t ri = rcvr.integerValue();
    oop_int_t ai = arg.integerValue();
    oop_int_t r  = ri * ai;
    if (ai == 0  ||  (r / ai  ==  ri    &&    Oop::isIntegerValue(r))) {
      internalPopThenPush(2, Oop::from_int(r));
      fetchNextBytecode();
      return;
    }
  }
  else {
    successFlag = true;
    externalizeIPandSP();
    {
      Safepoint_Ability sa(true);
      primitiveFloatMultiply(rcvr, arg);
    }
    internalizeIPandSP();
    if (successFlag) {
      fetchNextBytecode();
      return;
    }
  }
  roots.messageSelector = specialSelector(8);
  set_argumentCount(1);
  normalSend();
}
void Squeak_Interpreter::bytecodePrimDivide() {
  Oop rcvr = internalStackValue(1);
  Oop arg  = internalStackValue(0);
  if (areIntegers(rcvr, arg)) {
    oop_int_t ri = rcvr.integerValue();
    oop_int_t ai = arg.integerValue();
    if (ai != 0   &&   ri % ai  == 0) {
      oop_int_t r = ri / ai;
      if (Oop::isIntegerValue(r)) {
        internalPopThenPush(2, Oop::from_int(r));
        fetchNextBytecode();
        return;
      }
    }
  }
  else {
    successFlag = true;
    externalizeIPandSP();
    {
      Safepoint_Ability sa(true);
      primitiveFloatDivide(rcvr, arg);
    }
    internalizeIPandSP();
    if (successFlag) {
      fetchNextBytecode();
      return;
    }
  }
  roots.messageSelector = specialSelector(9);
  set_argumentCount(1);
  normalSend();
}

void Squeak_Interpreter::bytecodePrimMod() {
  successFlag = true;
  int mod = doPrimitiveMod(internalStackValue(1), internalStackValue(0));
  if (successFlag) {
    internalPopThenPush(2, Oop::from_int(mod));
    fetchNextBytecode();
    return;
  }
  roots.messageSelector = specialSelector(10);
  set_argumentCount(1);
  normalSend();
}

void Squeak_Interpreter::bytecodePrimLessThan() {
  Oop rcvr = internalStackValue(1);
  Oop arg  = internalStackValue(0);
  if (areIntegers(rcvr, arg)) {
    booleanCheat(rcvr.integerValue() < arg.integerValue());
    return;
  }
  else {
    successFlag = true;
    bool aBool = primitiveFloatLess(rcvr, arg);
    if (successFlag) {
      booleanCheat(aBool);
      return;
    }
  }
  roots.messageSelector = specialSelector(2);
  set_argumentCount(1);
  normalSend();
}
void Squeak_Interpreter::bytecodePrimGreaterThan() {
  Oop rcvr = internalStackValue(1);
  Oop arg  = internalStackValue(0);
  if (areIntegers(rcvr, arg)) {
    booleanCheat(rcvr.integerValue() > arg.integerValue());
    return;
  }
  else {
    successFlag = true;
    bool aBool = primitiveFloatGreater(rcvr, arg);
    if (successFlag) {
      booleanCheat(aBool);
      return;
    }
  }
  roots.messageSelector = specialSelector(3);
  set_argumentCount(1);
  normalSend();
}

void Squeak_Interpreter::bytecodePrimLessOrEqual() {
  Oop rcvr = internalStackValue(1);
  Oop arg  = internalStackValue(0);
  if (areIntegers(rcvr, arg)) {
    booleanCheat(rcvr.integerValue() <= arg.integerValue());
    return;
  }
  else {
    successFlag = true;
    bool aBool = !primitiveFloatGreater(rcvr, arg);
    if (successFlag) {
      booleanCheat(aBool);
      return;
    }
  }
  roots.messageSelector = specialSelector(4);
  set_argumentCount(1);
  normalSend();
}

void Squeak_Interpreter::bytecodePrimGreaterOrEqual() {
  Oop rcvr = internalStackValue(1);
  Oop arg  = internalStackValue(0);
  if (areIntegers(rcvr, arg)) {
    booleanCheat(rcvr.integerValue() >= arg.integerValue());
    return;
  }
  else {
    successFlag = true;
    bool aBool = !primitiveFloatLess(rcvr, arg);
    if (successFlag) {
      booleanCheat(aBool);
      return;
    }
  }
  roots.messageSelector = specialSelector(5);
  set_argumentCount(1);
  normalSend();
}

void Squeak_Interpreter::bytecodePrimEqual() {
  Oop rcvr = internalStackValue(1);
  Oop arg  = internalStackValue(0);
  if (areIntegers(rcvr, arg)) {
    booleanCheat(rcvr == arg);
    return;
  }
  else {
    successFlag = true;
    bool aBool = primitiveFloatEqual(rcvr, arg);
    if (successFlag) {
      booleanCheat(aBool);
      return;
    }
  }
  roots.messageSelector = specialSelector(6);
  set_argumentCount(1);
  normalSend();
}

void Squeak_Interpreter::bytecodePrimNotEqual() {
  Oop rcvr = internalStackValue(1);
  Oop arg  = internalStackValue(0);
  if (areIntegers(rcvr, arg)) {
    booleanCheat(rcvr != arg);
    return;
  }
  else {
    successFlag = true;
    bool aBool = !primitiveFloatEqual(rcvr, arg);
    if (successFlag) {
      booleanCheat(aBool);
      return;
    }
  }
  roots.messageSelector = specialSelector(7);
  set_argumentCount(1);
  normalSend();
}

void Squeak_Interpreter::bytecodePrimMakePoint() {
  successFlag = true;
  externalizeIPandSP();
  {
    Safepoint_Ability sa(true);
    primitiveMakePoint();
  }
  internalizeIPandSP();
  if (successFlag) {
    fetchNextBytecode();
    return;
  }
  roots.messageSelector = specialSelector(11);
  set_argumentCount(1);
  normalSend();
}

void Squeak_Interpreter::bytecodePrimBitShift() {
  successFlag = true;
  externalizeIPandSP();
  {
    Safepoint_Ability sa(true);
    primitiveBitShift();
  }
  internalizeIPandSP();
  if (successFlag) {
    fetchNextBytecode();
    return;
  }
  roots.messageSelector = specialSelector(12);
  set_argumentCount(1);
  normalSend();
}
void Squeak_Interpreter::bytecodePrimDiv() {
  successFlag = true;
  int32 quotient = doPrimitiveDiv(internalStackValue(1), internalStackValue(0));
  if (successFlag) {
    internalPopThenPush(2, Oop::from_int(quotient));
    fetchNextBytecode();
    return;
  }
  roots.messageSelector = specialSelector(13);
  set_argumentCount(1);
  normalSend();
}
void Squeak_Interpreter::bytecodePrimBitAnd() {
  successFlag = true;
  externalizeIPandSP();
  {
    Safepoint_Ability sa(true);
    primitiveBitAnd();
  }
  internalizeIPandSP();
  if (successFlag) {
    fetchNextBytecode();
    return;
  }
  roots.messageSelector = specialSelector(14);
  set_argumentCount(1);
  normalSend();
}
void Squeak_Interpreter::bytecodePrimBitOr() {
  successFlag = true;
  externalizeIPandSP();
  {
    Safepoint_Ability sa(true);
    primitiveBitOr();
  }
  internalizeIPandSP();
  if (successFlag) {
    fetchNextBytecode();
    return;
  }
  roots.messageSelector = specialSelector(15);
  set_argumentCount(1);
  normalSend();
}

void Squeak_Interpreter::bytecodePrimAt() {
  Oop index = internalStackTop();
  Oop rcvr = internalStackValue(1);
  successFlag = rcvr.is_mem() && index.is_int();
  if (successFlag) {
    At_Cache::Entry* e = atCache.get_entry(rcvr, false);
    if (e->matches(rcvr)) {
      Oop result = commonVariableAt(rcvr, index.integerValue(), e, true);
      if (successFlag) {
        fetchNextBytecode();
        internalPopThenPush(2, result);
        return;
      }
    }
  }
  roots.messageSelector = specialSelector(16);
  set_argumentCount(1);
  normalSend();
}

void Squeak_Interpreter::bytecodePrimAtPut() {
  Oop value = internalStackTop();
  Oop index = internalStackValue(1);
  Oop rcvr = internalStackValue(2);
  successFlag = rcvr.is_mem() && index.is_int();
  if (successFlag) {
    At_Cache::Entry* e = atCache.get_entry(rcvr, true);
    if (e->matches(rcvr)) {
      commonVariableAtPut(rcvr, index.integerValue(), value, e);
      if (successFlag) {
        fetchNextBytecode();
        internalPopThenPush(3, value);
        return;
      }
    }
  }
  roots.messageSelector = specialSelector(17);
  set_argumentCount( 2 );
  normalSend();
}

void Squeak_Interpreter::bytecodePrimSize() {
  roots.messageSelector = specialSelector(18);
  set_argumentCount(0);
  normalSend();
}
void Squeak_Interpreter::bytecodePrimNext() {
  roots.messageSelector = specialSelector(19);
  set_argumentCount(0);
  normalSend();
}
void Squeak_Interpreter::bytecodePrimNextPut() {
  roots.messageSelector = specialSelector(20);
  set_argumentCount(1);
  normalSend();
}
void Squeak_Interpreter::bytecodePrimAtEnd() {
  roots.messageSelector = specialSelector(21);
  set_argumentCount(0);
  normalSend();
}
void Squeak_Interpreter::bytecodePrimEquivalent() {
  booleanCheat(internalStackValue(1) == internalStackValue(0));
}

void Squeak_Interpreter::bytecodePrimClass() {
  internalPopThenPush(1, internalStackTop().fetchClass());
  fetchNextBytecode();
}

void Squeak_Interpreter::bytecodePrimBlockCopy() {
  Oop rcvr = internalStackValue(1);
  successFlag = true;
  success(rcvr.as_object()->hasContextHeader());
  if (successFlag) {
    externalizeIPandSP();
    {
      Safepoint_Ability sa(true);
      primitiveBlockCopy();
    }
    internalizeIPandSP();
  }
  if (!successFlag) {
    roots.messageSelector = specialSelector(24);
    set_argumentCount(1);
    normalSend();
    return;
  }
  fetchNextBytecode();
}

void Squeak_Interpreter::commonBytecodePrimValue(int nargs, int selector_index) {
  Oop block = localSP()[-nargs];
  successFlag = true;
  set_argumentCount(nargs);
  Oop klass = block.fetchClass();
  bool classOK = true;
  
# if Include_Closure_Support
  if (klass == splObj(Special_Indices::ClassBlockClosure)) {
    externalizeIPandSP();
    primitiveClosureValue();
    internalizeIPandSP();
  }
  else 
# endif
  if (klass == splObj(Special_Indices::ClassBlockContext)) {
    externalizeIPandSP();
    primitiveValue();
    internalizeIPandSP();
  } 
  else
    classOK = false;
  
  if (classOK && successFlag) 
    fetchNextBytecode();
  else {
    roots.messageSelector = specialSelector(selector_index);
    normalSend();
  }  
}

void Squeak_Interpreter::bytecodePrimValue() {
  commonBytecodePrimValue(0, 25);
}

void Squeak_Interpreter::bytecodePrimValueWithArg() {
  commonBytecodePrimValue(1, 26);
}

void Squeak_Interpreter::bytecodePrimDo() {
  roots.messageSelector = specialSelector(27);
  set_argumentCount(1);
  normalSend();
}
void Squeak_Interpreter::bytecodePrimNew() {
  roots.messageSelector = specialSelector(28);
  set_argumentCount(0);
  normalSend();
}
void Squeak_Interpreter::bytecodePrimNewWithArg() {
  roots.messageSelector = specialSelector(29);
  set_argumentCount(1);
  normalSend();
}

void Squeak_Interpreter::bytecodePrimPointX() {
  successFlag = true;
  Oop rcvr = internalStackTop();
  assertClass(rcvr, splObj(Special_Indices::ClassPoint));
  if (successFlag) {
    internalPopThenPush(1, rcvr.as_object()->fetchPointer(Object_Indices::XIndex));
    fetchNextBytecode();
    return;
  }
  roots.messageSelector = specialSelector(30);
  set_argumentCount(0);
  normalSend();
}

void Squeak_Interpreter::bytecodePrimPointY() {
  successFlag = true;
  Oop rcvr = internalStackTop();
  assertClass(rcvr, splObj(Special_Indices::ClassPoint));
  if (successFlag) {
    internalPopThenPush(1, rcvr.as_object()->fetchPointer(Object_Indices::YIndex));
    fetchNextBytecode();
    return;
  }
  roots.messageSelector = specialSelector(31);
  set_argumentCount(0);
  normalSend();
}


void Squeak_Interpreter::sendLiteralSelectorBytecode() {
	// "Can use any of the first 16 literals for the selector and pass up to 2 arguments."
  assert(method_obj()->isCompiledMethod());
	roots.messageSelector = literal(currentBytecode & 0xf);

  if (check_assertions && !roots.messageSelector.is_mem()) {
    Printer* p = error_printer;
    p->printf("on %d: msgSel is int; method bits 0x%x, method->obj 0x%x, method obj 0x%x, method obj as_oop 0x%x, msgSel 0x%x\n",
              Logical_Core::my_rank(), method().bits(), (Object*)method().as_object(), (Object*)method_obj(), method_obj()->as_oop().bits(), roots.messageSelector.bits());
    method_obj()->print(p);
    p->nl();
    method_obj()->print_compiled_method(p);
    p->nl();

    *(int*)0 = 17;
  }
  assert(roots.messageSelector.is_mem());
	set_argumentCount( ((currentBytecode >> 4) & 3) - 1 );
  normalSend();
}

# if Include_Closure_Support

void Squeak_Interpreter::pushNewArrayBytecode() {
  u_char size = fetchByte();
  bool popValues = size > 127;
  size &= 127;
  fetchNextBytecode();
  externalizeIPandSP();
  Object_p array_obj;
  {
    Safepoint_Ability sa(true);
    array_obj = splObj_obj(Special_Indices::ClassArray)->instantiateClass(size);
  }
  internalizeIPandSP();
  if (popValues) {
    for ( int i = 0;  i < size;  ++i )
      // Assume new Array is young, so use unchecked stores
      array_obj->storePointerUnchecked(i, internalStackValue(size - i - 1));
    internalPop(size);
  }
  internalPush(array_obj->as_oop());
}


void Squeak_Interpreter::pushRemoteTempLongBytecode() {
  u_char remoteTempIndex = fetchByte();
  u_char tempVectorIndex = fetchByte();
  fetchNextBytecode();
  pushRemoteTempInVectorAt(remoteTempIndex, tempVectorIndex);
}

void Squeak_Interpreter::storeRemoteTempLongBytecode() {
  u_char remoteTempIndex = fetchByte(); // which temp on stack
  u_char tempVectorIndex = fetchByte(); // which 0-origin index into vector
  fetchNextBytecode();
  storeRemoteTempInVectorAt(remoteTempIndex, tempVectorIndex);
}

void Squeak_Interpreter::storeAndPopRemoteTempLongBytecode() {
  storeRemoteTempLongBytecode();
  internalPop(1);
}


void Squeak_Interpreter::pushRemoteTempInVectorAt(u_char indexIntoVector, u_char  indexOfVectorIntoContext) {
  Oop tempVector = temporary(indexOfVectorIntoContext);
  internalPush(tempVector.as_object()->fetchPointer(indexIntoVector));
}

void Squeak_Interpreter::storeRemoteTempInVectorAt(u_char indexIntoVector, u_char  indexOfVectorIntoContext) {
  Oop tempVector = temporary(indexOfVectorIntoContext);
  tempVector.as_object()->storePointer(indexIntoVector, internalStackTop());
}


void Squeak_Interpreter::pushClosureCopyCopiedValuesBytecode() {
  /* "The compiler has pushed the values to be copied, if any.  Find numArgs 
      and numCopied in the byte following.
      Create a Closure with space for the copiedValues and pop numCopied
      values off the stack into the closure.
      Set numArgs as specified, and set startpc to the pc following the block
      size and jump over that code."*/
  
  image_version = Squeak_Image_Reader::Post_Closure_32_Bit_Image_Version;

  u_char numArgsNumCopied = fetchByte();
  u_int32 numArgs   = numArgsNumCopied & 0xf;
  u_int32 numCopied = numArgsNumCopied >> (u_int32)4;
  
  u_int32 blockSize = fetchByte() << 8;
  blockSize += (u_int32)fetchByte();
  
  externalizeIPandSP();
  Oop newClosure = closureCopy(numArgs, instructionPointer() + 2 - (method_obj()->as_u_char_p() + Object::BaseHeaderSize), numCopied);
  // Recover from GC, but no Object* 's

  internalizeIPandSP();
  Object_p newClosure_obj = newClosure.as_object();
  newClosure_obj->storePointerUnchecked(Object_Indices::ClosureOuterContextIndex, activeContext());
  reclaimableContextCount = 0; // The closure refers to thisContext so it cannot be reclaimed
  if (numCopied > 0) {
    for (u_int32 i = 0;  i < numCopied;  ++i )
      newClosure_obj->storePointerUnchecked(i + Object_Indices::ClosureFirstCopiedValueIndex, internalStackValue( numCopied - i - 1));
    internalPop(numCopied);
  }
  set_localIP(localIP() + blockSize);
  fetchNextBytecode();
  internalPush(newClosure);
}


Oop Squeak_Interpreter::closureCopy(u_int32 numArgs, u_int32 initialIP, u_int32 numCopied) {
  Object_p newClosure_obj;
  {
    Safepoint_Ability sa(true);
    newClosure_obj = splObj_obj(Special_Indices::ClassBlockClosure)->instantiateSmallClass(
                (Object_Indices::ClosureFirstCopiedValueIndex + numCopied) * sizeof(Oop)  +  Object::BaseHeaderSize);
  }
  // Assume young, use unchecked
  newClosure_obj->storePointerUnchecked(Object_Indices::ClosureStartPCIndex, Oop::from_int(initialIP)),
  newClosure_obj->storePointerUnchecked(Object_Indices::ClosureNumArgsIndex, Oop::from_int(numArgs));
  return newClosure_obj->as_oop();
}


int Squeak_Interpreter::remoteTempLong_literal_index(u_char* bcp) {
  return bcp[1]; // also bcp[2] but my framework doesn't support it
}

int Squeak_Interpreter::pushRemoteTempLongBytecode_literal_index(u_char* bcp) { return remoteTempLong_literal_index(bcp); }
int Squeak_Interpreter::storeRemoteTempLongBytecode_literal_index(u_char* bcp) { return remoteTempLong_literal_index(bcp); }
int Squeak_Interpreter::storeAndPopRemoteTempLongBytecode_literal_index(u_char* bcp) { return remoteTempLong_literal_index(bcp); }
# endif


int Squeak_Interpreter::extendedStoreBytecode_literal_index(u_char* bcp) {
  u_char d = bcp[1];
  u_char vi = d & 63;
  if (((d >> 6) & 3) == 3) return vi;
  return -1;
}

int Squeak_Interpreter::singleExtendedSendBytecode_literal_index(u_char* bcp) {
  u_char d = bcp[1];
  return d & 0x1f;
}

int Squeak_Interpreter::doubleExtendedDoAnythingBytecode_literal_index(u_char* bcp) {
  u_char b2 = bcp[1];
  u_char b3 = bcp[2];
  switch (b2 >> 5) {
      default: return -1;
      case 0: return b3;
      case 1: return b3;
      case 7: return b3;
  }
}

int Squeak_Interpreter::singleExtendedSuperBytecode_literal_index(u_char* bcp) {
  return bcp[1] & 0x1f;
}

int Squeak_Interpreter::secondExtendedSendBytecode_literal_index(u_char* bcp) {
  u_char descriptor = bcp[1];
  return descriptor & 0x3f;
}


int Squeak_Interpreter::sendLiteralSelectorBytecode_literal_index(u_char* bcp) {
  return *bcp & 0xf;
}

