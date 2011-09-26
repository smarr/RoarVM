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
#include "squeak_adapters.h"



extern "C" {
    sqInt  argumentCountOf(sqInt methodPointer);
  void*  arrayValueOf(sqInt oop);
  sqInt becomewith(sqInt array1, sqInt array2);
    sqInt  booleanValueOf(sqInt obj);
  sqInt byteSizeOf(sqInt);
  sqInt byteSwapped(sqInt);
    sqInt characterTable(void);
  sqInt checkedIntegerValueOf(sqInt);
    sqInt classAlien(void);
    sqInt classArray(void);
    sqInt classBitmap(void);
  sqInt classByteArray(void);
    sqInt classCharacter(void);
  sqInt classExternalAddress(void);
  sqInt classExternalData(void);
  sqInt classExternalFunction(void);
  sqInt classExternalLibrary(void);
  sqInt classExternalStructure(void);
    sqInt classFloat(void);
  sqInt classLargeNegativeInteger(void);
    sqInt classLargePositiveInteger(void);
    sqInt classPoint(void);
    sqInt classSemaphore(void);
    sqInt classSmallInteger(void);
    sqInt classString(void);
  sqInt sqr_clone(sqInt oop);
  sqInt copyBits();
  sqInt copyBitsFromtoat(sqInt, sqInt, sqInt);
  sqInt displayObject();
  void   error(char*);
  sqInt failed(void);
    sqInt falseObject(void);
  void*  fetchArrayofObject(sqInt fieldIndex, sqInt objectPointer);
  sqInt fetchClassOf(sqInt oop);
  double fetchFloatofObject(sqInt fieldIndex, sqInt objectPointer);
  sqInt fetchIntegerofObject(sqInt, sqInt);
  sqInt  fetchLong32ofObject(sqInt index, sqInt oop);
  sqInt fetchPointerofObject(sqInt, sqInt);
    sqInt  floatObjectOf(double aFloat);
  void   forceInterruptCheck(sqInt);
  void*  firstFixedField(sqInt oop);
  void*  firstIndexableField(sqInt);
  double floatValueOf(sqInt oop);
  void   fullDisplayUpdate();
  void  tenuringIncrementalGC();
  sqInt fullGC(void);
  sqInt getInterruptKeycode();
  sqInt getNextWakeupTick();
  sqInt ioWhicheverMSecs();
  sqInt getSavedWindowSize();
  sqInt getThisSessionID();
  sqInt incrementalGC(void);
  sqInt instanceSizeOf(sqInt);
  sqInt instantiateClassindexableSize(sqInt, sqInt);
  sqInt  integerObjectOf(sqInt value);
  sqInt  integerValueOf(sqInt oop);
  sqInt isBytes(sqInt);
    sqInt isFloatObject(sqInt oop);
    sqInt isInMemory(sqInt address);
    sqInt isKindOf(sqInt oop, char *aString);
  sqInt isKindOfClass(sqInt oop, sqInt aClass);

  sqInt includesBehaviorThatOf(sqInt aClass, sqInt aSuperClass);
  sqInt isArray(sqInt oop);
  sqInt isIndexable(sqInt oop);
  sqInt isIntegerObject(sqInt objectPointer);
  sqInt isIntegerValue(sqInt intValue);
  sqInt isMemberOf(sqInt oop, char *aString);
  sqInt isPointers(sqInt oop);
  sqInt isWeak(sqInt oop);
  sqInt isWords(sqInt oop);
  sqInt isWordsOrBytes(sqInt oop);
  sqInt lengthOf(sqInt);
    sqInt  literalCountOf(sqInt methodPointer);
  sqInt  literalofMethod(sqInt offset, sqInt methodPointer);
    sqInt loadBitBltFrom(sqInt bbOop);
    sqInt makePointwithxValueyValue(sqInt xValue, sqInt yValue);
  sqInt  methodArgumentCount(void);
  sqInt  methodPrimitiveIndex(void);
  sqInt  methodReturnValue(sqInt oop);
  sqInt nilObject(void);
  sqInt  primitiveIndexOf(sqInt methodPointer);
  sqInt  obsoleteDontUseThisFetchWordofObject(sqInt index, sqInt oop);
  sqInt pop(sqInt);
  sqInt popRemappableOop(void);
  sqInt topRemappableOop(void);  
  sqInt  popthenPush(sqInt nItems, sqInt oop);
  sqInt positive32BitIntegerFor(sqInt);
  sqInt  positive32BitValueOf(sqInt oop);
  sqInt  positive64BitIntegerFor(sqLong integerValue);
  sqLong positive64BitValueOf(sqInt oop);
  sqInt primitiveFail();
  sqInt primitiveFailFor(sqInt);
  sqInt primitiveFailureCode();
    sqInt primitiveMethod(void);
  sqInt push(sqInt);
  sqInt pushBool(sqInt);
  sqInt  pushFloat(double f);
  sqInt  pushInteger(sqInt integerValue);
  sqInt pushRemappableOop(sqInt oop);
  sqInt setInterruptCheckCounter(sqInt);
  sqInt setInterruptPending(sqInt);
  sqInt setSavedWindowSize(sqInt);
  sqInt showDisplayBitsLeftTopRightBottom(sqInt aForm, sqInt l, sqInt t, sqInt r, sqInt b);
  sqInt signalSemaphoreWithIndex(sqInt);
  sqInt  signed32BitIntegerFor(sqInt integerValue);
  sqInt  signed32BitValueOf(sqInt oop);
    sqInt  signed64BitIntegerFor(sqLong integerValue);
  sqLong signed64BitValueOf(sqInt oop);
    sqInt  sizeOfSTArrayFromCPrimitive(void *cPtr);
  sqInt  slotSizeOf(sqInt oop);
  double stackFloatValue(sqInt offset);
  sqInt stackIntegerValue(sqInt);
  sqInt stackObjectValue(sqInt);
  sqInt  stackValue(sqInt offset);
  sqInt  stObjectat(sqInt array, sqInt fieldIndex);
  sqInt  stObjectatput(sqInt array, sqInt fieldIndex, sqInt value);
  sqInt  storeIntegerofObjectwithValue(sqInt index, sqInt oop, sqInt integer);
  sqInt  storePointerofObjectwithValue(sqInt index, sqInt oop, sqInt valuePointer);
  sqInt  stSizeOf(sqInt oop);
  sqInt success(sqInt);
  sqInt superclassOf(sqInt classPointer);
  sqInt trueObject(void);
  sqInt  vmEndianness(void);

  char *pointerForOop(sqInt oop);
  sqInt oopForPointer(char *ptr);

  void   browserProcessCommand(void);
  sqInt display_primitivePluginBrowserReady(void);
  sqInt display_primitivePluginRequestURLStream(void);
  sqInt display_primitivePluginRequestURL(void);
  sqInt display_primitivePluginPostURL(void);
  sqInt display_primitivePluginRequestFileHandle(void);
  sqInt display_primitivePluginDestroyRequest(void);
  sqInt display_primitivePluginRequestState(void);

  bool verify_heap(void);

  void print_vm_info(void);
  void signal_emergency_semaphore(void);
}


const char* interpreterVersion = "Renaissance";


sqInt  argumentCountOf(sqInt methodPointer) {
  return Oop::from_bits(methodPointer).as_object()->argumentCount();
}

void*  arrayValueOf(sqInt oop) { return Oop::from_bits(oop).arrayValue(); }


sqInt becomewith(sqInt /* array1 */, sqInt /* array2 */) {
  unimpExt(); return 0;
}

sqInt  booleanValueOf(sqInt obj) {
  return The_Squeak_Interpreter()->booleanValueOf(Oop::from_bits(obj));
}

sqInt byteSizeOf(sqInt i) { return Oop::from_bits(i).as_object()->byteSize(); }

sqInt byteSwapped(sqInt i) {assert(sizeof(sqInt) == sizeof(int32)); swap_bytes_long((int32*)&i); return i; }

sqInt characterTable(void) { return The_Squeak_Interpreter()->splObj(Special_Indices::CharacterTable).bits(); }

sqInt  checkedIntegerValueOf(sqInt intOop) { return Oop::from_bits(intOop).checkedIntegerValue(); }


sqInt classAlien(void) { return The_Squeak_Interpreter()->splObj(Special_Indices::ClassAlien).bits(); }
sqInt classArray(void) { return The_Squeak_Interpreter()->splObj(Special_Indices::ClassArray).bits(); }
sqInt classBitmap(void) { return The_Squeak_Interpreter()->splObj(Special_Indices::ClassBitmap).bits(); }
sqInt classByteArray() { return The_Squeak_Interpreter()->splObj(Special_Indices::ClassByteArray).bits(); }
sqInt classCharacter(void) { return The_Squeak_Interpreter()->splObj(Special_Indices::ClassCharacter).bits(); }

sqInt classExternalAddress() { return The_Squeak_Interpreter()->splObj(Special_Indices::ClassExternalAddress).bits(); }
sqInt classExternalData() { return The_Squeak_Interpreter()->splObj(Special_Indices::ClassExternalData).bits(); }
sqInt classExternalFunction() { return The_Squeak_Interpreter()->splObj(Special_Indices::ClassExternalFunction).bits(); }
sqInt classExternalLibrary() { return The_Squeak_Interpreter()->splObj(Special_Indices::ClassExternalLibrary).bits(); }
sqInt classExternalStructure() { return The_Squeak_Interpreter()->splObj(Special_Indices::ClassExternalStructure).bits(); }

sqInt classFloat(void) { return The_Squeak_Interpreter()->splObj(Special_Indices::ClassFloat).bits(); }
sqInt classLargeNegativeInteger(void) { return The_Squeak_Interpreter()->splObj(Special_Indices::ClassLargeNegativeInteger).bits(); }
sqInt classLargePositiveInteger(void) { return The_Squeak_Interpreter()->splObj(Special_Indices::ClassLargePositiveInteger).bits(); }
sqInt classPoint(void) { return The_Squeak_Interpreter()->splObj(Special_Indices::ClassPoint).bits(); }
sqInt classSemaphore(void) { return The_Squeak_Interpreter()->splObj(Special_Indices::ClassSemaphore).bits(); }
sqInt classSmallInteger(void) { return The_Squeak_Interpreter()->splObj(Special_Indices::ClassInteger).bits(); }
sqInt classString(void) { return The_Squeak_Interpreter()->classString()->as_oop().bits(); }

# undef clone

sqInt sqr_clone(sqInt oop) {
  return Oop::from_bits(oop).as_object()->clone().bits();
}

sqInt copyBits() { The_Squeak_Interpreter()->copyBits(); return 0; }

sqInt copyBitsFromtoat(sqInt x0, sqInt x1, sqInt y) {
  The_Squeak_Interpreter()->copyBitsFromtoat(x0, x1, y);
  return 0;
}


sqInt displayObject() { return The_Squeak_Interpreter()->displayObject().bits(); }

void error(char* s) { assert_always_msg(0, s); }

sqInt failed() { return The_Squeak_Interpreter()->failed(); }

void*  fetchArrayofObject(sqInt fieldIndex, sqInt objectPointer) { return Oop::from_bits(objectPointer).as_object()->fetchArray(fieldIndex); }

sqInt falseObject() { return The_Squeak_Interpreter()->roots.falseObj.bits(); }


sqInt  fetchClassOf(sqInt oop) { return Oop::from_bits(oop).fetchClass().bits(); }

double fetchFloatofObject(sqInt fieldIndex, sqInt objectPointer) {
  return Oop::from_bits(objectPointer).as_object()->fetchFloatofObject(fieldIndex);
}

sqInt fetchIntegerofObject(sqInt x, sqInt y) { return Oop::from_bits(y).as_object()->fetchInteger(x); }

sqInt  fetchLong32ofObject(sqInt index, sqInt oop) { return Oop::from_bits(oop).as_object()->fetchLong32(index); }

sqInt fetchPointerofObject(sqInt x, sqInt y) {
  Oop r = Oop::from_bits(y).as_object()->fetchPointer(x);
  if (check_many_assertions) r.verify_oop();
  return r.bits();
}

sqInt  floatObjectOf(double aFloat) { return Object::floatObject(aFloat).bits(); }

void* firstFixedField(sqInt oop) { return Oop::from_bits(oop).as_object()->firstFixedField(); }

void* firstIndexableField(sqInt oop) { return Oop::from_bits(oop).as_object()->firstIndexableField_for_primitives(); }

double floatValueOf(sqInt oop) { return The_Squeak_Interpreter()->floatValueOf(Oop::from_bits(oop)); }

void forceInterruptCheck(sqInt) { The_Squeak_Interpreter()->forceInterruptCheck(); }

void fullDisplayUpdate() { The_Squeak_Interpreter()->fullDisplayUpdate(); }

sqInt fullGC() { The_Memory_System()->fullGC("fullGC from original Squeak code"); return 0; }

sqInt getInterruptKeycode() { return The_Squeak_Interpreter()->interruptKeycode(); }

sqInt getNextWakeupTick() { return The_Squeak_Interpreter()->nextWakeupTick(); }

sqInt ioWhicheverMSecs() { return The_Squeak_Interpreter()->ioWhicheverMSecs(); }

sqInt getSavedWindowSize() { return The_Memory_System()->snapshot_window_size.savedWindowSize(); }

sqInt getThisSessionID() { return The_Squeak_Interpreter()->globalSessionID; }

sqInt incrementalGC(void) { The_Memory_System()->incrementalGC(); return 0; }
void  tenuringIncrementalGC(void) { The_Memory_System()->incrementalGC(); }


sqInt instantiateClassindexableSize(sqInt k, sqInt s) { return Oop::from_bits(k).as_object()->instantiateClass(s)->as_oop().bits(); }

sqInt instanceSizeOf(sqInt classObj) { return Oop::from_bits(classObj).as_object()->instanceSizeOfClass(); }

sqInt  integerObjectOf(sqInt v) { return Oop::from_int(v).bits(); }

sqInt  integerValueOf(sqInt oop) { return Oop::from_bits(oop).integerValue(); }

sqInt isBytes(sqInt x) { return Oop::from_bits(x).isBytes(); }

sqInt isFloatObject(sqInt oop) { return Oop::from_bits(oop).is_mem() && Oop::from_bits(oop).as_object()->isFloatObject(); }

sqInt isInMemory(sqInt /* address */) { unimpExt(); return 1;}

sqInt isKindOf(sqInt oop, char *aString) { return Oop::from_bits(oop).isKindOf(aString); }
sqInt isKindOfClass(sqInt /* oop */, sqInt /* aClass */)  { unimpExt(); return 0; }  // STEFAN TODO: necessary for the FFI I think


sqInt includesBehaviorThatOf(sqInt /* aClass */, sqInt /* aSuperClass */) { unimpExt(); return 0; }
sqInt isArray(sqInt oop) { return Oop::from_bits(oop).isArray(); }
sqInt isIndexable(sqInt oop) { return Oop::from_bits(oop).isIndexable(); }
sqInt isIntegerObject(sqInt objectPointer) { return Oop::from_bits(objectPointer).is_int(); }
sqInt isIntegerValue(sqInt intValue) { return Oop::isIntegerValue((oop_int_t)intValue); }
sqInt isMemberOf(sqInt oop, char *aString) { return Oop::from_bits(oop).isMemberOf(aString); }
sqInt isPointers(sqInt oop) { return Oop::from_bits(oop).isPointers(); }
sqInt isWeak(sqInt oop) { return Oop::from_bits(oop).isWeak(); }
sqInt isWords(sqInt oop) { return Oop::from_bits(oop).isWords(); }
sqInt isWordsOrBytes(sqInt oop) { return Oop::from_bits(oop).isWordsOrBytes(); }






sqInt lengthOf(sqInt x)  {  return Oop::from_bits(x).as_object()->lengthOf(); }

sqInt  literalCountOf(sqInt methodPointer) { return Oop::from_bits(methodPointer).as_object()->literalCount(); }

sqInt  literalofMethod(sqInt offset, sqInt methodPointer) { return Oop::from_bits(methodPointer).as_object()->literal(offset).bits(); }

sqInt loadBitBltFrom(sqInt /* bbOop */) { unimpExt(); return 0; }

sqInt makePointwithxValueyValue(sqInt xValue, sqInt yValue) { return Object::makePoint(xValue, yValue)->as_oop().bits(); }


sqInt  methodArgumentCount(void) { return The_Squeak_Interpreter()->methodArgumentCount(); }
sqInt  methodPrimitiveIndex(void)  { return The_Squeak_Interpreter()->methodPrimitiveIndex(); }
sqInt  methodReturnValue(sqInt /* oop */) { unimpExt(); return 0; }

sqInt nilObject() { return The_Squeak_Interpreter()->roots.nilObj.bits(); }

sqInt  obsoleteDontUseThisFetchWordofObject(sqInt /* index */, sqInt /* oop */) {unimpExt(); return 0;}

sqInt popRemappableOop(void) { return The_Squeak_Interpreter()->popRemappableOop().bits(); }
sqInt topRemappableOop(void) { return The_Squeak_Interpreter()->topRemappableOop().bits(); }

sqInt  popthenPush(sqInt nItems, sqInt oop) { The_Squeak_Interpreter()->popThenPush(nItems, Oop::from_bits(oop)); return 0; }

sqInt  positive32BitIntegerFor(sqInt x) { return Object::positive32BitIntegerFor(x).bits(); }

sqInt  positive64BitIntegerFor(sqLong x) { return Object::positive64BitIntegerFor(x).bits(); }

sqInt  positive32BitValueOf(sqInt oop) { return The_Squeak_Interpreter()->positive32BitValueOf(Oop::from_bits(oop)); }

sqLong positive64BitValueOf(sqInt oop) { return The_Squeak_Interpreter()->positive64BitValueOf(Oop::from_bits(oop)); }

sqInt  primitiveIndexOf(sqInt methodPointer) { return Oop::from_bits(methodPointer).as_object()->primitiveIndex(); }

sqInt primitiveFail() { The_Squeak_Interpreter()->primitiveFail(); return 0; }
sqInt primitiveFailFor(sqInt reasonCode) { 
  The_Squeak_Interpreter()->primitiveFailFor(reasonCode);
  return 0;
}
sqInt primitiveFailureCode() { return The_Squeak_Interpreter()->primFailCode; }

sqInt primitiveMethod(void) { unimpExt(); return 0; }

sqInt pop(sqInt x) {  The_Squeak_Interpreter()->pop(x);  return 0; }


sqInt push(sqInt x) { The_Squeak_Interpreter()->push(Oop::from_bits(x)); return 0; }

sqInt pushBool(sqInt b)  { The_Squeak_Interpreter()->pushBool(b); return 0; }

sqInt pushFloat(double f) { The_Squeak_Interpreter()->pushFloat(f); return 0; }
sqInt pushInteger(sqInt integerValue) { The_Squeak_Interpreter()->pushInteger(integerValue); return 0; }
sqInt pushRemappableOop(sqInt oop) { The_Squeak_Interpreter()->pushRemappableOop(Oop::from_bits(oop)); return 0; }


sqInt setInterruptCheckCounter(sqInt i) {
  The_Squeak_Interpreter()->interruptCheckCounter = i;
  return 0; }

sqInt setInterruptPending(sqInt i) {
  The_Squeak_Interpreter()->set_interruptPending(i);
  return 0; }

sqInt setSavedWindowSize(sqInt s) { The_Memory_System()->snapshot_window_size.savedWindowSize(s); return 0; }

sqInt  signed32BitIntegerFor(sqInt integerValue) { return Object::signed32BitIntegerFor(integerValue).bits(); }

sqInt  signed64BitIntegerFor(sqLong integerValue) { return Object::signed64BitIntegerFor(integerValue).bits(); }

sqInt  signed32BitValueOf(sqInt oop) { return The_Squeak_Interpreter()->signed32BitValueOf(Oop::from_bits(oop)); }

sqLong signed64BitValueOf(sqInt oop) { return The_Squeak_Interpreter()->signed64BitValueOf(Oop::from_bits(oop)); }


sqInt showDisplayBitsLeftTopRightBottom(sqInt aForm, sqInt l, sqInt t, sqInt r, sqInt b) {
  The_Squeak_Interpreter()->showDisplayBitsOf(Oop::from_bits(aForm), l, t, r, b);
  return 0;
}


sqInt signalSemaphoreWithIndex(sqInt i) { The_Squeak_Interpreter()->signalSemaphoreWithIndex(i); return 0; }

sqInt  sizeOfSTArrayFromCPrimitive(void *cPtr) {
  return Object::sizeOfSTArrayFromCPrimitive(cPtr);
}

sqInt  slotSizeOf(sqInt oop) { return Oop::from_bits(oop).as_object()->slotSize(); }

double stackFloatValue(sqInt offset) { return The_Squeak_Interpreter()->stackFloatValue(offset); }


sqInt stackIntegerValue(sqInt i) { return The_Squeak_Interpreter()->stackIntegerValue(i); }

sqInt stackObjectValue(sqInt i) { return The_Squeak_Interpreter()->stackObjectValue(i).bits(); }

sqInt  stackValue(sqInt offset) { return The_Squeak_Interpreter()->stackValue(offset).bits(); }

sqInt  stObjectat(sqInt array, sqInt fieldIndex) {
return The_Squeak_Interpreter()->stObjectAt(Oop::from_bits(array).as_object(), fieldIndex).bits(); }

sqInt  stObjectatput(sqInt array, sqInt fieldIndex, sqInt value) {
  The_Squeak_Interpreter()->stObjectAtPut(Oop::from_bits(array).as_object(), fieldIndex, Oop::from_bits(value)); return 0; }

sqInt  storeIntegerofObjectwithValue(sqInt index, sqInt oop, sqInt integer) {
  Oop::from_bits(oop).as_object()->storeInteger(index, integer);  return 0;
}

sqInt  storePointerofObjectwithValue(sqInt index, sqInt oop, sqInt valuePointer) {
  Oop::from_bits(oop).as_object()->storePointer(index, Oop::from_bits(valuePointer));  return 0;
}


sqInt stSizeOf(sqInt oop) { return Oop::from_bits(oop).as_object()->stSize(); }

sqInt success(sqInt b) { The_Squeak_Interpreter()->success(b); return The_Squeak_Interpreter()->successFlag; }

sqInt superclassOf(sqInt classPointer) { return Oop::from_bits(classPointer).as_object()->superclass().bits(); }

sqInt trueObject() { return The_Squeak_Interpreter()->roots.trueObj.bits(); }

sqInt  vmEndianness(void) { unimpExt(); return 0; }

extern "C" {
sqInt ioFilenamefromStringofLengthresolveAliases(char* aCharBuffer, char* filenameIndex, sqInt filenameLength, sqInt resolveFlag);
}

sqInt ioFilenamefromStringofLengthresolveAliases(char* aCharBuffer, char* filenameIndex, sqInt filenameLength, sqInt resolveFlag) {
  return The_Squeak_Interpreter()->ioFilenamefromStringofLengthresolveAliases(aCharBuffer, filenameIndex, filenameLength, resolveFlag); }



char *pointerForOop(sqInt oop)			{ return Oop::from_bits(oop).as_object()->pointerForOop_for_primitives(); }
sqInt oopForPointer(char *ptr)			  { return (sqInt) ((Object*)ptr)->as_oop().bits(); }

// squeak code used to use pointerForOop for these
char* pointerForIndex_xxx_dmu(sqInt index)  {
  if (check_assertions && index)
    assert( ((Object*)index)->my_heap_contains_me());
  return (char*)index;
}


typedef struct {
  char *pluginName;
  char *primitiveName;
  void *primitiveAddress;
} sqExport;

// xxx_dmu include these in compilation?
sqExport vm_exports[1];

# if !On_iOS
  sqExport os_exports[1];
  sqExport ADPCMCodecPlugin_exports[1];
  sqExport BMPReadWriterPlugin_exports[1];
  sqExport DSAPrims_exports[1];
  sqExport ZipPlugin_exports[1];
  sqExport FFTPlugin_exports[1];
  sqExport GeniePlugin_exports[1];
  sqExport JPEGReaderPlugin_exports[1];
  sqExport Klatt_exports[1];
  sqExport StarSqueakPlugin_exports[1];
  sqExport XDisplayControlPlugin_exports[1];
# endif

# if Configure_Squeak_Code_for_Tilera
sqExport UnixOSProcessPlugin_exports[1];
# endif

extern "C" {extern void* sqGetInterpreterProxy();}


void* interpreterProxy;

void initip() {
interpreterProxy = sqGetInterpreterProxy();
}


void   browserProcessCommand(void) {}

sqInt display_primitivePluginBrowserReady(void) { return 0; }
sqInt display_primitivePluginRequestURLStream(void) { return 0; }
sqInt display_primitivePluginRequestURL(void) { return 0; }
sqInt display_primitivePluginPostURL(void) { return 0; }
sqInt display_primitivePluginRequestFileHandle(void) { return 0; }
sqInt display_primitivePluginDestroyRequest(void) { return 0; }
sqInt display_primitivePluginRequestState(void) { return 0; }


bool verify_heap() { return The_Memory_System()->verify(); }

void print_vm_info() { The_Squeak_Interpreter()->print_info(); }
void signal_emergency_semaphore() { The_Squeak_Interpreter()->signal_emergency_semaphore(); }

