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


/*
 "This table generates a C function address table use in primitiveResponse along with dispatchFunctionPointerOn:in:"

"NOTE: The real limit here is 2047 because of the method header layout but there is no point in going over the needed size"
MaxPrimitiveIndex := 575.
 */

class Primitive_Table: public Abstract_Primitive_Table {
public:
  Primitive_Table();
private:
  void init_main(int i, fn_t f) {contents[i] = f; execute_on_main[i] = true; }
  void init_here(int i, fn_t f) {contents[i] = f; execute_on_main[i] = false; }
  void init_here(int i, int j, fn_t f) { while(i <= j) { execute_on_main[i] = false; contents[i++] = f;} }
};
extern Primitive_Table primitiveTable;

# if Include_Closure_Support

  # define FOR_ALL_CLOSURE_PRIMITIVES_DO(template) \
    template(primitiveClosureCopyWithCopiedValues) \
    template(primitiveClosureValue) \
    template (primitiveClosureValueWithArgs) \
    template(primitiveClosureValueNoContextSwitch)

  # else
  # define FOR_ALL_CLOSURE_PRIMITIVES_DO(template)

# endif

#define FOR_ALL_PRIMITIVES_DO(template) \
\
FOR_ALL_CLOSURE_PRIMITIVES_DO(template) \
\
template(clearProfile) \
template(dumpProfile) \
template(primitiveAdd) \
template(primitiveArctan) \
template(primitiveArrayBecome) \
template(primitiveArrayBecomeOneWay) \
template(primitiveArrayBecomeOneWayCopyHash) \
template(primitiveAsFloat) \
template(primitiveAsOop) \
template(primitiveAt) \
template(primitiveAtEnd) \
template(primitiveAtPut) \
template(primitiveBeCursor) \
template(primitiveBeDisplay) \
template(primitiveBeep) \
template(primitiveBitAnd) \
template(primitiveBitOr) \
template(primitiveBitShift) \
template(primitiveBitXor) \
template(primitiveBlockCopy) \
template(primitiveBytesLeft) \
template(primitiveCalloutToFFI) \
template(primitiveChangeClass) \
template(primitiveClass) \
template(primitiveClipboardText) \
template(primitiveClone) \
template(primitiveConstantFill) \
template(primitiveCopyObject) \
template(primitiveDeferDisplayUpdates) \
template(primitiveDiv) \
template(primitiveDivide) \
template(primitiveDoPrimitiveWithArgs) \
template(primitiveEqual) \
template(primitiveEquivalent) \
template(primitiveExecuteMethod) \
template(primitiveExecuteMethodArgsArray) \
template(primitiveExitToDebugger) \
template(primitiveExponent) \
template(primitiveExp) \
template(primitiveExternalCall) \
template(primitiveFail) \
template(primitiveFindHandlerContext) \
template(primitiveFindNextUnwindContext) \
template(primitiveFloatAdd) \
template(primitiveFloatDivide) \
template(primitiveFloatEqual) \
template(primitiveFloatGreaterOrEqual) \
template(primitiveFloatGreaterThan) \
template(primitiveFloatLessOrEqual) \
template(primitiveFloatLessThan) \
template(primitiveFloatMultiply) \
template(primitiveFloatNotEqual) \
template(primitiveFloatSubtract) \
template(primitiveFlushCache) \
template(primitiveFlushCacheByMethod) \
template(primitiveFlushCacheSelective) \
template(primitiveFlushExternalPrimitives) \
template(primitiveForceDisplayUpdate) \
template(primitiveFormPrint) \
template(primitiveFractionalPart) \
template(primitiveFullGC) \
template(primitiveGetAttribute) \
template(primitiveGetNextEvent) \
template(primitiveGreaterOrEqual) \
template(primitiveGreaterThan) \
template(primitiveImageName) \
template(primitiveIncrementalGC) \
template(primitiveInputSemaphore) \
template(primitiveInputWord) \
template(primitiveInstVarAt) \
template(primitiveInstVarAtPut) \
template(primitiveInstVarsPutFromStack) \
template(primitiveIntegerAt) \
template(primitiveIntegerAtPut) \
template(primitiveInterruptSemaphore) \
template(primitiveInvokeObjectAsMethod) \
template(primitiveKbdNext) \
template(primitiveKbdPeek) \
template(primitiveLessOrEqual) \
template(primitiveLessThan) \
template(primitiveListBuiltinModule) \
template(primitiveListExternalModule) \
template(primitiveLoadImageSegment) \
template(primitiveLoadInstVar) \
template(primitiveLogN) \
template(primitiveLowSpaceSemaphore) \
template(primitiveMakePoint) \
template(primitiveMarkHandlerMethod) \
template(primitiveMarkUnwindMethod) \
template(primitiveMillisecondClock) \
template(primitiveMod) \
template(primitiveMouseButtons) \
template(primitiveMousePoint) \
template(primitiveMultiply) \
template(primitiveNew) \
template(primitiveNewMethod) \
template(primitiveNewWithArg) \
template(primitiveNext) \
template(primitiveNextInstance) \
template(primitiveNextObject) \
template(primitiveNextPut) \
template(primitiveNoop) \
template(primitiveNotEqual) \
template(primitiveObjectAt) \
template(primitiveObjectAtPut) \
template(primitiveObjectPointsTo) \
template(primitiveObsoleteIndexedPrimitive) \
template(primitivePerform) \
template(primitivePerformInSuperclass) \
template(primitivePerformWithArgs) \
template(primitivePushFalse) \
template(primitivePushMinusOne) \
template(primitivePushNil) \
template(primitivePushOne) \
template(primitivePushSelf) \
template(primitivePushTrue) \
template(primitivePushTwo) \
template(primitivePushZero) \
template(primitiveQuit) \
template(primitiveQuo) \
template(primitiveRelinquishProcessor) \
template(primitiveResume) \
template(primitiveScanCharacters) \
template(primitiveScreenSize) \
template(primitiveSecondsClock) \
template(primitiveSetDisplayMode) \
template(primitiveSetFullScreen) \
template(primitiveSetInterruptKey) \
template(primitiveShortAt) \
template(primitiveShortAtPut) \
template(primitiveShowDisplayRect) \
template(primitiveSignal) \
template(primitiveSignalAtBytesLeft) \
template(primitiveSignalAtMilliseconds) \
template(primitiveSine) \
template(primitiveSize) \
template(primitiveSnapshot) \
template(primitiveSnapshotEmbedded) \
template(primitiveSomeInstance) \
template(primitiveSomeObject) \
template(primitiveSpecialObjectsOop) \
template(primitiveSquareRoot) \
template(primitiveStoreImageSegment) \
template(primitiveStoreStackp) \
template(primitiveStringAt) \
template(primitiveStringAtPut) \
template(primitiveStringReplace) \
template(primitiveSubtract) \
template(primitiveSuspend) \
template(primitiveTerminateTo) \
template(primitiveTestDisplayDepth) \
template(primitiveTimesTwoPower) \
template(primitiveTruncated) \
template(primitiveUnloadModule) \
template(primitiveVMParameter) \
template(primitiveVMPath) \
template(primitiveValue) \
template(primitiveValueUninterruptably) \
template(primitiveValueWithArgs) \
template(primitiveWait) \
template(primitiveYield) \
template(startProfiling) \
template(stopProfiling) \
template(primitiveDoNamedPrimitiveWithArgs)

# define DCL(n) void* n(...);

FOR_ALL_PRIMITIVES_DO(DCL)
# undef DCL

