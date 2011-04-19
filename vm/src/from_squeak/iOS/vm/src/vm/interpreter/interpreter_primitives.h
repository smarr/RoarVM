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


// Included into the middle of squeak_interpreter.h

void primitiveAdd();
void primitiveArrayBecome();
void primitiveArrayBecomeOneWay();
void primitiveArrayBecomeOneWayCopyHash();
void primitiveArctan();
void primitiveAsFloat();
void primitiveAsOop();
void primitiveAt();
void primitiveAtEnd();
void primitiveAtPut();
void primitiveBeCursor();
void primitiveBeDisplay();
void primitiveBeep();
void primitiveBitAnd();
void primitiveBitOr();
void primitiveBitShift();
void primitiveBitXor();
void primitiveBlockCopy();
void primitiveBytesLeft();
void primitiveCalloutToFFI();
void primitiveChangeClass();
void primitiveClass();
void primitiveClipboardText();
void primitiveClone();
void primitiveConstantFill();
void primitiveCopyObject();
void primitiveDeferDisplayUpdates();
void primitiveDiv();
void primitiveDivide();
void primitiveDoPrimitiveWithArgs();
void primitiveDoNamedPrimitiveWithArgs();
void primitiveEqual();
void primitiveEquivalent();
void primitiveExecuteMethod();
void primitiveExecuteMethodArgsArray();
void primitiveExitToDebugger();
void primitiveExponent();
void primitiveExp();
void primitiveExternalCall();
void primitiveFindHandlerContext();
void primitiveFindNextUnwindContext();
void primitiveFloatAdd();
void primitiveFloatMultiply();
void primitiveFloatDivide();
void primitiveFloatLess();
void primitiveFloatGreater();
void primitiveFloatEqual();
void primitiveFloatGreaterOrEqual();
void primitiveFloatGreaterThan();
void primitiveFloatLessOrEqual();
void primitiveFloatLessThan();
void primitiveFloatNotEqual();
void primitiveFloatSubtract();
void primitiveFlushCache();
void primitiveFlushCacheByMethod();
void primitiveFlushCacheSelective();
void primitiveFlushExternalPrimitives();
void primitiveForceDisplayUpdate();
void primitiveFormPrint();
void primitiveFractionalPart();
void primitiveFullGC();
void primitiveGetAttribute();
void primitiveGetNextEvent();
void primitiveGreaterOrEqual();
void primitiveGreaterThan();
void primitiveImageName();
void primitiveIncrementalGC();
void primitiveInputSemaphore();
void primitiveInputWord();
void primitiveInstVarAt();
void primitiveInstVarAtPut();
void primitiveInstVarsPutFromStack();
void primitiveIntegerAt();
void primitiveIntegerAtPut();
void primitiveInterruptSemaphore();
void primitiveInvokeObjectAsMethod();
void primitiveKbdNext();
void primitiveKbdPeek();
void primitiveLessOrEqual();
void primitiveLessThan();
void primitiveListBuiltinModule();
void primitiveListExternalModule();
void primitiveLoadImageSegment();
void primitiveLoadInstVar();
void primitiveLogN();
void primitiveLowSpaceSemaphore();
void primitiveMakePoint();
void primitiveMarkHandlerMethod();
void primitiveMarkUnwindMethod();
void primitiveMillisecondClock();
void primitiveMod();
void primitiveMouseButtons();
void primitiveMousePoint();
void primitiveMultiply();
void primitiveNew();
void primitiveNewMethod();
void primitiveNewWithArg();
void primitiveNext();
void primitiveNextInstance();
void primitiveNextObject();
void primitiveNextPut();
void primitiveNoop();
void primitiveNotEqual();
void primitiveObjectAt();
void primitiveObjectAtPut();
void primitiveObjectPointsTo();
void primitiveObsoleteIndexedPrimitive();
void primitivePerform();
void primitivePerformInSuperclass();
void primitivePerformWithArgs();
void primitivePushFalse();
void primitivePushMinusOne();
void primitivePushNil();
void primitivePushOne();
void primitivePushSelf();
void primitivePushTrue();
void primitivePushTwo();
void primitivePushZero();
void primitiveQuit();
void primitiveQuo();
void primitiveRelinquishProcessor();
void primitiveResume();
void primitiveScanCharacters();
void primitiveScreenSize();
void primitiveSecondsClock();
void primitiveSetDisplayMode();
void primitiveSetFullScreen();
void primitiveSetGCSemaphore();
void primitiveSetInterruptKey();
void primitiveShortAt();
void primitiveShortAtPut();
void primitiveShowDisplayRect();
void primitiveSignal();
void primitiveSignalAtBytesLeft();
void primitiveSignalAtMilliseconds();
void primitiveSine();

void primitiveSize();

void primitiveSnapshot();
void primitiveSnapshotEmbedded();
void primitiveSomeInstance();
void primitiveSomeObject();
void primitiveSpecialObjectsOop();
void primitiveSquareRoot();
void primitiveStoreImageSegment();
void primitiveStoreStackp();
void primitiveStringAt();
void primitiveStringAtPut();
void primitiveStringReplace();
void primitiveSubtract();
void primitiveSuspend();
void primitiveTerminateTo();
void primitiveTestDisplayDepth();
void primitiveTimesTwoPower();
void primitiveTruncated();
void primitiveUnloadModule();
void primitiveVMParameter();
void primitiveVMPath();
void primitiveValue();
void primitiveValueUninterruptably();
void primitiveValueWithArgs();
# if Include_Closure_Support
void primitiveClosureValue();
void primitiveClosureCopyWithCopiedValues();
void primitiveClosureValueWithArgs();
void primitiveClosureValueNoContextSwitch();
# endif
void primitiveWait();
void primitiveYield();
void startProfiling();
void stopProfiling();


void clearProfile();
void dumpProfile();


void primitiveFloatAdd(Oop r, Oop a);
void primitiveFloatSubtract(Oop r, Oop a);
void primitiveFloatMultiply(Oop r, Oop a);
void primitiveFloatDivide(Oop r, Oop a);
bool primitiveFloatLess(Oop r, Oop a);
bool primitiveFloatGreater(Oop r, Oop a);
bool primitiveFloatEqual(Oop r, Oop a);

