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


inline void Object::set_object_address_and_backpointer(Oop x  COMMA_DCL_ESB) {
  Safepoint_for_moving_objects::assert_held();
  The_Memory_System()->object_table->set_object_for(x, (Object_p)this  COMMA_USE_ESB);
  set_backpointer(x);
}




inline void Object::set_class_oop(Oop x) {
  The_Memory_System()->store_enforcing_coherence(&class_and_type_word(),
                                              Header_Type::extract_from(class_and_type_word())
                                              |  Header_Type::without_type(x.bits()),
                                              (Object_p)this);
}

inline void Object::set_class_oop_no_barrier(Oop x) {
  class_and_type_word() = Header_Type::extract_from(class_and_type_word())
                       |  Header_Type::without_type(x.bits());
}

inline void Object::  mark_without_store_barrier() { baseHeader |=  MarkBit; }
inline void Object::unmark_without_store_barrier() { baseHeader &= ~MarkBit; }



inline void Object::set_backpointer_word(oop_int_t w) {
  oop_int_t* dst = backpointer_word();
  The_Memory_System()->store_enforcing_coherence(dst, w, (Object_p)this);
}

inline void Object::set_extra_preheader_word(oop_int_t w) {
  assert_always(w); // bug hunt qqq
  oop_int_t* dst = extra_preheader_word();
  The_Memory_System()->store_enforcing_coherence(dst, w, (Object_p)this);
}

inline bool Object::hasSender(Oop aContext) {
  // rcvr must be a context
  Object_p aco = aContext.as_object();
  if (this == (Object*)aco)  return false;
  Oop nilOop = The_Squeak_Interpreter()->roots.nilObj;
  for (Oop s = fetchPointer(Object_Indices::SenderIndex);
       s != nilOop;
       s = s.as_object()->fetchPointer(Object_Indices::SenderIndex))
    if (s == aContext)  return true;
  return false;
}



inline oop_int_t Object::fetchInteger(oop_int_t fieldIndex) {
  Oop x = fetchPointer(fieldIndex);
  return x.checkedIntegerValue();
}

inline oop_int_t Object::fetchStackPointer() { // rcvr is a ContextObject
  Oop sp = fetchPointer(Object_Indices::StackPointerIndex);
  return sp.is_int() ? sp.integerValue() : 0;
}


inline u_oop_int_t Object::fixedFieldsOfArray() {
  /* "NOTE: This code supports the backward-compatible extension to 8 bits of instSize.
   When we revise the image format, it should become...
   ^ (classFormat >> 2 bitAnd: 16rFF) - 1 */

  oop_int_t fmt = format();
  return
  Format::has_only_indexable_fields(fmt)
  ?  0 :
  Format::has_only_fixed_fields(fmt)
  ? lengthOf()  :  // "fixed fields only (zero or more)"
  // "fmt = 3 or 4: mixture of fixed and indexable fields, so must look at class format word"
  ClassFormat::fixedFields(fetchClass().as_object()->formatOfClass());
}


inline u_oop_int_t Object::lengthOf() {
  // Return the number of indexable bytes or words in the given object. Assume the given oop is not an integer. For a CompiledMethod, the size of the method header (in bytes) should be subtracted from the result of this method."
  u_oop_int_t sz = sizeBits();
  if (Size4Bit) sz -= baseHeader & Size4Bit;
  sz -= BaseHeaderSize;
  oop_int_t fmt = format();
  return  fmt <= Format::both_fixed_and_indexable_weak_fields
  ?  sz >> ShiftForWord // words
  :  fmt <= Format::indexable_long_fields_only
  ?  sz >> 2 // 32-bit longs
  :  sz - (fmt & Format::byte_size_bits); // bytes
}


inline Oop Object::fetchClass() {
  oop_int_t ccIndex = compact_class_index();
  return ccIndex == 0  ?  get_class_oop()
  :  The_Squeak_Interpreter()->splObj(Special_Indices::CompactClasses).as_object()->fetchPointer(ccIndex - 1);
}


inline oop_int_t Object::lastPointer() {
  /*
   "Return the byte offset of the last pointer field of the given object.
   Works with CompiledMethods, as well as ordinary objects.
   Can be used even when the type bits are not correct."
   */
  int fmt = format();
  // ordered this way for optimization
  if (Format::has_only_oops(fmt)) {
    if (Format::might_be_context(fmt) && hasContextHeader())
      return  (Object_Indices::CtextTempFrameStart + fetchStackPointer()) * bytesPerWord;

    return  sizeBitsSafe() - sizeof(Oop);
  }
  if (Format::has_no_oops(fmt))
    return  0;
  if (Format::isCompiledMethod(fmt))
    return  literalCount() * bytesPerWord  +  BaseHeaderSize;

  fatal("should not be here");  return 0;
}


inline char* Object::first_byte_address() {
  if (isFreeObject()) return NULL;
  int32 fmt = format();
  if (!Format::has_bytes(fmt)) return NULL;
  return  first_byte_address_after_header()
  + (Format::isCompiledMethod(fmt)
     ? (Object_Indices::LiteralStart + literalCount()) * bytesPerWord
     : 0);
}

inline char* Object::first_byte_address_after_header() { return as_char_p() + BaseHeaderSize; }

inline oop_int_t Object::methodHeader() { return fetchPointer(Object_Indices::HeaderIndex).bits(); }

inline void Object::byteSwapIfByteObject() {
  char* b = first_byte_address();
  if (b == NULL) return;
  reverseBytes((int32*)b, (int32*)nextChunk());
  // no store barrier, only for reading snapshots
}

inline oop_int_t Object::total_byte_size() {
  oop_int_t r = bytes_to_next_chunk() + extra_header_bytes();
  if (check_many_assertions) assert(!(r & (sizeof(oop_int_t) - 1)));
  return r;
}

inline oop_int_t Object::total_byte_size_without_preheader() {
  return bytes_to_next_chunk() + extra_header_bytes_without_preheader();
}

inline oop_int_t Object::sizeBits() {
  // "Answer the number of bytes in the given object, including its base header, rounded up to an integral number of words."
  // "Note: byte indexable objects need to have low bits subtracted from this size."
  if (check_many_assertions) assert_always(!isFreeObject());
  oop_int_t r = contains_sizeHeader()
                  ?  longSizeBits()
                  :  shortSizeBits();
  if (check_many_assertions) assert_always((size_t)r >= sizeof(baseHeader));
  return r;
}
inline oop_int_t Object::sizeBitsSafe() {
  // "Compute the size of the given object from the cc and size fields in its header. This works even if its type bits are not correct."
  oop_int_t header = baseHeader;
  return rightType(header) ==  Header_Type::SizeAndClass
  ?  Header_Type::without_type(sizeHeader())
  :  shortSizeBits();
}

inline oop_int_t Object::stSize() {
  // return number of indexable fields in given object, e.g. ST size
  return Format::might_be_context(format()) && hasContextHeader()
  ? fetchStackPointer()
  : oop_int_t(lengthOf() - fixedFieldsOfArray());
}

inline int Object::rightType(oop_int_t headerWord) {
  // "Compute the correct header type for an object based on the size and compact class fields of the given base header word, rather than its type bits. This is used during marking, when the header type bits are used to record the state of tracing."
  if (!(headerWord & SizeMask        ))  return Header_Type::SizeAndClass;
  if (!(headerWord & CompactClassMask))  return Header_Type::Class;
  return Header_Type::Short;
}

inline void* Object::arrayValue() {
  return isWordsOrBytes() ? as_char_p() + BaseHeaderSize : (char*)(The_Squeak_Interpreter()->primitiveFail(), 0);
}


inline oop_int_t Object::formatOfClass() {
  /* "**should be in-lined**"
   "Note that, in Smalltalk, the instSpec will be equal to the inst spec
   part of the base header of an instance (without hdr type) shifted left 1.
   In this way, apart from the smallInt bit, the bits
   are just where you want them for the first header word."
   "Callers expect low 2 bits (header type) to be zero!"*/
  return fetchPointer(Object_Indices::InstanceSpecificationIndex).bits() & ~Int_Tag;
}

inline oop_int_t Object::quickFetchInteger(oop_int_t fieldIndex) { return fetchPointer(fieldIndex).integerValue(); }

inline oop_int_t Object::fetchLong32Length() {
  // Gives size appropriate for fetchLong32
  return (sizeBits() - BaseHeaderSize) >> 2;
}


inline u_char& Object::byte_at(oop_int_t byteIndex) { return as_u_char_p()[BaseHeaderSize + byteIndex]; }
inline u_char  Object::fetchByte(oop_int_t byteIndex) { return byte_at(byteIndex); }
inline void    Object::storeByte( oop_int_t byteIndex, u_char valueByte) {
  The_Memory_System()->store_enforcing_coherence(&byte_at(byteIndex), valueByte, (Object_p)this); // used in interpreter, mostly for new objects
}


inline Oop& Object::pointer_at(oop_int_t fieldIndex) {
  return as_oop_p()[BaseHeaderSize / sizeof(Oop)  +  fieldIndex];
}

inline Oop  Object::fetchPointer(oop_int_t fieldIndex) {
  assert(fieldIndex >= 0); // STEFAN that should always hold, shouldn't it?
  Oop r;
  MEASURE(fetch_pointer, r.bits(), r = pointer_at(fieldIndex));
  return r;
}


inline void Object::catch_stores_of_method_in_home_ctxs(Oop* /* addr */, int n,  Oop x) {
# if Extra_OTE_Words_for_Debugging_Block_Context_Method_Change_Bug
  if (n != Object_Indices::MethodIndex)  return;
  if (get_count_of_blocks_homed_to_this_method_ctx() <= 0)   return;
  lprintf("caught storePointer of method in Oop 0x%x, changing method 0x%x to 0x%x\n",
          as_oop().bits(), fetchPointer(Object_Indices::MethodIndex).bits(), x.bits());
# endif
}


inline void Object::storePointer( oop_int_t fieldIndex, Oop oop) {
  Oop* addr = &pointer_at(fieldIndex);
  catch_stores_of_method_in_home_ctxs(addr, fieldIndex, oop);
  The_Memory_System()->store_enforcing_coherence(addr, oop, (Object_p)this);
}
inline void Object::storePointerUnchecked( oop_int_t fieldIndex, Oop oop) {
  // "Like storePointer:ofObject:withValue:, 
  //  but the caller guarantees that the object being stored into
  //  is a young object or is already marked as a root."
  
  // Must NOT send any messages; may be called with safepoint ability true, but caller is not safe.
  Oop* addr = &pointer_at(fieldIndex);
  catch_stores_of_method_in_home_ctxs(addr, fieldIndex, oop);
  The_Memory_System()->store_enforcing_coherence(addr, oop, (Object_p)this);
}

void Object::storePointerIntoContext(oop_int_t fieldIndex, Oop x) {
  Oop* addr = &pointer_at(fieldIndex);
  catch_stores_of_method_in_home_ctxs(addr, fieldIndex, x);
  DEBUG_STORE_CHECK(addr, x);
  *addr = x;
}


inline int32& Object::long32_at(oop_int_t fieldIndex)  { return as_int32_p()[BaseHeaderSize / sizeof(int32)  +  fieldIndex]; }
inline int32  Object::fetchLong32(oop_int_t fieldIndex) {
  // " index by 32-bit units, and return a 32-bit value. Intended to replace fetchWord:ofObject:"
  return long32_at(fieldIndex);
}
inline void   Object::storeLong32(oop_int_t fieldIndex, int32 x) {
  The_Memory_System()->store_enforcing_coherence(&long32_at(fieldIndex), x, (Object_p)this);
}

inline void   Object::storeInteger(oop_int_t fieldIndex, oop_int_t x) {
  if (Oop::isIntegerValue(x))  storePointerUnchecked(fieldIndex, Oop::from_int(x));
  else                         The_Squeak_Interpreter()->primitiveFail();
}


inline Object_p Object::instantiateSmallClass(oop_int_t sizeInBytes) {
  /* "This version of instantiateClass assumes that the total object
   size is under 256 bytes, the limit for objects with only one or
   two header words. Note that the size is specified in bytes
   and should include 4 or 8 bytes for the base header word.
   NOTE this code will only work for sizes that are an integral number of words
   (like not a 32-bit LargeInteger in a 64-bit system).
   May cause a GC.
   Note that the created small object IS NOT FILLED and must be completed before returning it to Squeak. Since this call is used in routines that do jsut that we are safe. Break this rule and die."
   */

  if (sizeInBytes & (bytesPerWord - 1))  { fatal("size must be integral number of words"); }
  Multicore_Object_Heap* h = The_Memory_System()->heaps[Logical_Core::my_rank()][Memory_System::read_write];
  oop_int_t hash = h->newObjectHash();
	oop_int_t header1 = ((hash << HashBitsOffset) & HashBits)  |  formatOfClass();
	Oop header2 = as_oop();
  int hdrSize =
  (header1 & CompactClassMask) > 0 // "is this a compact class"
  ? 1  :  2;

	header1 += sizeInBytes - (header1 & (SizeMask+Size4Bit));
  return h->allocate( sizeInBytes, hdrSize, header1, header2, 0);
}



inline bool Object::isPointers() {
  //	"Answer true if the argument has only fields that can hold oops. See comment in formatOf:"
  return Format::has_only_oops(format());
}

inline bool Object::isBytes() {
  // Answer true if the argument contains indexable bytes. See comment in formatOf:"
  //  "Note: Includes CompiledMethods."
  return Format::has_bytes(format());
}

inline bool Object::isArray() {
	return Format::isArray(format());
}

inline bool Object::isWordsOrBytes() {
  return Format::isWordsOrBytes(format());
}



inline bool Object::isCompiledMethod() { return Format::isCompiledMethod(format()); }

inline bool Object::isFloatObject() { return fetchClass() == The_Squeak_Interpreter()->splObj(Special_Indices::ClassFloat); }


inline oop_int_t Object::primitiveIndex() {
	// "Note: We now have 10 bits of primitive index, but they are in two places
	// for temporary backward compatibility.  The time to unpack is negligible,
	// since the reconstituted full index is stored in the method cache."
  return primitiveIndex_of_header(methodHeader());
}

inline oop_int_t Object::literalCount() {  return Object::literalCountOfHeader(methodHeader()); }
inline oop_int_t Object::literalCountOfHeader(oop_int_t header) { return (header >> Object_Indices::LiteralCountShift) & Object_Indices::LiteralCountMask; }

inline Oop Object::literal(oop_int_t offset) {
  Oop r = fetchPointer(offset + Object_Indices::LiteralStart);
  if (check_many_assertions) {
    assert_always(r.is_int() || The_Memory_System()->object_table->probably_contains((void*)r.bits()));
  }
  return r;
}

inline oop_int_t Object::argumentCountOfHeader(oop_int_t header) { return (header >> Object_Indices::ArgumentCountShift) & Object_Indices::ArgumentCountMask; }

inline oop_int_t Object::temporaryCountOfHeader(oop_int_t header) { return (header >> Object_Indices::TemporaryCountShift) & Object_Indices::TemporaryCountMask; }


inline void Object::flushExternalPrimitive() {
	// this is a CompiledMethod containing an external primitive. Flush the function address and session ID of the CM"
  Object_p lit = get_external_primitive_literal_of_method();
  if (lit == NULL)  return; //  "Something's broken"
  lit->cleanup_session_ID_and_ext_prim_index_of_external_primitive_literal();
}


inline Object_p Object::fill_in_after_allocate(oop_int_t byteSize, oop_int_t hdrSize,
                                       oop_int_t baseHeader, Oop classOop, oop_int_t extendedSize,
                                       bool doFill,
                                       bool fillWithNil) {
  const int my_rank = Logical_Core::my_rank();
  if (check_many_assertions  &&  hdrSize > 1)
    classOop.verify_oop();
  // since new allocs are in read_write heap, no need to mark this for moving to read_write
  assert(The_Memory_System()->contains(this));

  Preheader* preheader_p = (Preheader*)this;
  oop_int_t* headerp = (oop_int_t*)&preheader_p[1];
  Object_p    newObj = (Object_p)(Object*)&headerp[hdrSize - 1];
  assert(The_Memory_System()->is_address_read_write(this)); // not going to bother with coherence

  Multicore_Object_Heap* h = The_Memory_System()->heaps[my_rank][Memory_System::read_write];
  assert(h == my_heap()  ||  Safepoint_for_moving_objects::is_held());

  if (hdrSize == 3) {
    oop_int_t contents = extendedSize     |  Header_Type::SizeAndClass;
    DEBUG_STORE_CHECK(headerp, contents);
    *headerp++ = contents;
    
    contents = classOop.bits()  |  Header_Type::SizeAndClass;
    DEBUG_STORE_CHECK(headerp, contents);
    *headerp++ = contents;
    
    h->record_class_header((Object*)headerp, classOop);
    
    contents   = baseHeader       |  Header_Type::SizeAndClass;
    DEBUG_STORE_CHECK(headerp, contents);
    *headerp   = contents;
  }
  else if (hdrSize == 2) {
    oop_int_t contents = classOop.bits()  |  Header_Type::Class;
    DEBUG_STORE_CHECK(headerp, contents);
    *headerp++ = contents;
    
    h->record_class_header((Object*)headerp, classOop);

    contents   = baseHeader       |  Header_Type::Class;
    DEBUG_STORE_CHECK(headerp, contents);
    *headerp   = contents;
  }
  else {
    assert_eq(hdrSize, 1, "");
    
    oop_int_t contents   = baseHeader       |  Header_Type::Short;
    DEBUG_STORE_CHECK(headerp, contents);
    *headerp   = contents;
  }
  assert_eq((void*)newObj, (void*)headerp, "");

  The_Memory_System()->object_table->allocate_oop_and_set_preheader(newObj, my_rank  COMMA_TRUE_OR_NOTHING);


  //  "clear new object"
  if (!doFill)
    ;
  else if (fillWithNil) // assume it's an oop if not null
    h->multistore((Oop*)&headerp[1],
                  (Oop*)&headerp[byteSize >> ShiftForWord],
                  The_Squeak_Interpreter()->roots.nilObj);
  else {
    DEBUG_MULTISTORE_CHECK( &headerp[1], 0, (byteSize - sizeof(*headerp)) / bytes_per_oop);
    bzero(&headerp[1], byteSize - sizeof(*headerp));
  }

  The_Memory_System()->enforce_coherence_after_store_into_object_by_interpreter(this, byteSize);

  if (check_assertions) {
    newObj->okayOop();
    newObj->hasOkayClass();
  }
  return newObj;
}


inline Object_p Object::instantiateContext(oop_int_t  sizeInBytes ) {
  /*
   "This version of instantiateClass assumes that the total object
   size is under 256 bytes, the limit for objects with only one or
   two header words. Note that the size is specified in bytes
   and should include four bytes for the base header word."
   */
  Multicore_Object_Heap* h = The_Memory_System()->heaps[Logical_Core::my_rank()][Memory_System::read_write];
	int hash = h->newObjectHash();
  oop_int_t	header1 = ((hash << HashShift) & HashMask) | formatOfClass();
	Oop header2 = as_oop();
	int hdrSize =
  header1 & CompactClassMask  // "are contexts compact?"
  ? 1 : 2;
  header1 &= ~SizeMask;
	if (sizeInBytes <= SizeMask)
    //  "OR size into header1.  Must not do this if size > SizeMask"
    header1 |= sizeInBytes;
  else
    hdrSize = 3;
  // why never small context size?, Cause h3 is only used when large
  return h->allocate(sizeInBytes, hdrSize, header1, header2, sizeInBytes);
}


inline Oop Object::superclass() {
  return fetchPointer(Object_Indices::SuperclassIndex);
}


inline void Object::synchronousSignal(const char* why) {
  assert(The_Squeak_Interpreter()->safepoint_ability->is_unable());

  bool added = false;
  Oop proc_to_resume;
  bool will_resume = false;
  {
    Semaphore_Mutex sm("synchronousSignal");
    if (isEmptyList()) {
      // no proc waiting
      int excessSignals = fetchInteger(Object_Indices::ExcessSignalsIndex);
      storeInteger(Object_Indices::ExcessSignalsIndex, excessSignals + 1);
    }
    else {
      // must surrender sema before resuming to avoid deadlock
      // inside resume, could spin on safepoint
      added = true;
      will_resume = true;
      proc_to_resume = removeFirstLinkOfList();
    }
  }
  if (will_resume)
      The_Squeak_Interpreter()->resume(proc_to_resume, why);
  if (added)
    addedScheduledProcessMessage_class().send_to_other_cores(); // must be outside the semaphore to avoid deadlock
}



inline bool Object::isUnwindMarked() {
  // is this a methodcontext whose method has prim 198?
  return isMethodContext()
  && fetchPointer(Object_Indices::MethodIndex).as_object()->primitiveIndex() == 198;
}

inline double Object::fetchFloatAtinto() {
  // assumes arg is BaseHeaderSize, built into long32_at
  int32 r[2];
  r[0] = long32_at(1);
  r[1] = long32_at(0);
  return *((double*)&r);
}

inline double Object::fetchFloatofObject(oop_int_t fieldIndex) {
  return The_Squeak_Interpreter()->floatValueOf(fetchPointer(fieldIndex).as_object());
}



inline Oop Object::floatObject(double d) {
  Object_p r = The_Squeak_Interpreter()->splObj_obj(Special_Indices::ClassFloat)
  ->instantiateSmallClass(sizeof(double) + BaseHeaderSize);
  r->storeFloat(d);
  return r->as_oop();
}

inline void Object::storeFloat(double d) {
  The_Memory_System()->store_2_enforcing_coherence(
    &as_int32_p()[0 + BaseHeaderSize/sizeof(int32)], ((int32*)&d)[1],((int32*)&d)[0], (Object_p)this);
}

/** Floats are stored in platform order in Cog images.
    This function here is used during image load to make sure that
    the floats are stored in normalized, i.e., swaped order, since
    the standard interpreter and the RoarVM do not use platform order.
 
 REM: should NOT be called in normal operation. */
inline void Object::swapFloatParts_for_cog_compatibility() {
  int32* data = &as_int32_p()[0 + BaseHeaderSize/sizeof(int32)];
  
  The_Memory_System()->store_2_enforcing_coherence(
    data, data[1], data[0], (Object_p)this);
}


inline bool Object::equals_string(const char* s) {
  return strlen(s) == lengthOf()  &&  strncmp(s, first_byte_address(), lengthOf()) == 0;
}

inline bool Object::starts_with_string(const char* s) {
  return strncmp(s, first_byte_address(), strlen(s)) == 0;
}



inline oop_int_t Object::byteLength() {
  // Return the number of indexable bytes in the given object. This is basically a special copy of lengthOf: for BitBlt.
  oop_int_t sz = sizeBits();
  int fmt = format();
  return sz - BaseHeaderSize - (Format::has_bytes(fmt) ? (fmt & 3) : 0);
}



inline oop_int_t Object::argumentCountOfBlock() {
  return fetchPointer(Object_Indices::BlockArgumentCountIndex).checkedIntegerValue();
}

# if Include_Closure_Support
inline oop_int_t Object::argumentCountOfClosure() {
  return quickFetchInteger(Object_Indices::ClosureNumArgsIndex);
}
# endif


inline void* Object::firstIndexableField_for_primitives() {
  // problematic for store barrier; lots of C code uses this
  if (is_read_mostly()) {
    The_Squeak_Interpreter()->remember_to_move_mutated_read_mostly_object(as_oop());
  }
  oop_int_t fmt = format();
  int sz = Format::has_bytes(fmt)
  ? 1
  :  fmt == Format::indexable_word_fields_only
  ? sizeof(int32) : sizeof(oop_int_t);
  void* r = as_char_p()  +  BaseHeaderSize  +  fixedFieldsOfArray() * sz;
  return r;
}


inline char* Object::pointerForOop_for_primitives() {
  // problematic for store barrier; lots of C code uses this
  if (is_read_mostly()) {
    The_Squeak_Interpreter()->remember_to_move_mutated_read_mostly_object(as_oop());
  }
  return as_char_p();
}


inline oop_int_t Object::sizeOfSTArrayFromCPrimitive(void* p) {
  Object* x = (Object*)((char*)p - BaseHeaderSize);
  return x->isWordsOrBytes() ? x->lengthOf() : 0;
}

inline int Object::rank() { return The_Memory_System()->rank_for_address(this); }
inline int Object::mutability() { return The_Memory_System()->mutability_for_address(this); }

inline bool Object::is_read_write() { return The_Memory_System()->is_address_read_write(this); }
inline bool Object::is_read_mostly() { return The_Memory_System()->is_address_read_mostly(this); }

inline Multicore_Object_Heap* Object::my_heap() {
  return The_Memory_System()->heap_containing(this);
}


inline void Object::beRootIfOld() {
  if (true) return;
#ifdef _REMEMBER_OLD_CODE // below is old code
  unimplemented();
  if (is_new()) return;
  FOR_EACH_OOP_IN_OBJECT_EXCEPT_CLASS(this, oop_ptr)
    if (oop_ptr->is_new())
      ; // remember
    if (contains_class_and_type_word()  &&  get_class_oop().as_object()->is_new())
      ;
#endif
}


inline bool Object::is_suitable_for_replication() {
  // no forms: bitblt primitives
  // no contexts; lots of optimizations

  // Because now each core looks at scheduler, semaphores, processes, these much be coherent
  Oop klass = fetchClass();
  if (klass == The_Squeak_Interpreter()->splObj(Special_Indices::ClassProcess  )) return false;
  if (klass == The_Squeak_Interpreter()->splObj(Special_Indices::ClassSemaphore)) return false;
  if (klass == The_Squeak_Interpreter()->roots.sched_list_class                 ) return false;


  return (The_Memory_System()->replicate_methods &&  isCompiledMethod())
    ||   (The_Memory_System()->replicate_all     && !hasContextHeader());
}


inline int Object::mutability_for_snapshot_object() {

  // compiler bug:
  static const int c = Memory_System::read_write;
  static const int i = Memory_System::read_mostly;

  // Used to be is_suitable_for_replication() before multithreading, but now
  // need to exclude certainly classes that we don't know till AFTER reading the snapshot -- dmu 3/30/09
  // So, put everything in read_write, and let image move objects to read_mostly later. -- dmu 5/25/10
  // bool repl =  is_suitable_for_replication();
  const bool repl = false;

  return repl ? i :  c;
}



inline int Object::priority_of_process() {
  assert(fetchPointer(Object_Indices::PriorityIndex).is_int());
  return quickFetchInteger(Object_Indices::PriorityIndex);
}

inline Oop Object::name_of_process() {
  assert(fetchPointer(Object_Indices::ProcessName).is_mem());
  return fetchPointer(Object_Indices::ProcessName);
}

inline Oop Object::my_list_of_process() {
  assert(fetchPointer(Object_Indices::MyListIndex).is_mem());
  return fetchPointer(Object_Indices::MyListIndex);
}



inline void Object::save_block_method_and_IP() {
  // used to find bug where a block ctx home's method got changed because reclaimableContextCount was not zeroed in transferTo
# if Extra_OTE_Words_for_Debugging_Block_Context_Method_Change_Bug
  Oop h = fetchPointer(Object_Indices::HomeIndex); Object* ho = h.as_object();
  Oop me = as_oop();
  The_Memory_System()->object_table->set_dbg_y(me,ho->fetchPointer(Object_Indices::MethodIndex).bits());
  The_Memory_System()->object_table->set_dbg_z(me,(oop_int_t)ho);
  ho->set_count_of_blocks_homed_to_this_method(ho->get_count_of_blocks_homed_to_this_method_ctx() + 1);
  //The_Memory_System()->object_table->set_dbg_t(me,fetchPointer(Object_Indices::InstructionPointerIndex).bits());
# endif

}

inline Oop Object::get_orig_block_method() {
# if Extra_OTE_Words_for_Debugging_Block_Context_Method_Change_Bug
  return Oop::from_bits(The_Memory_System()->object_table->get_dbg_y(as_oop()));
# else
  return Oop::from_int(-1);
# endif
}

inline void Object::zapping_ctx() {
// called when zapping a ctx to help find the bug
# if Extra_OTE_Words_for_Debugging_Block_Context_Method_Change_Bug
  if (isMethodContext()) return;
  Oop h = fetchPointer(Object_Indices::HomeIndex); Object* ho = h.as_object();
  ho->set_count_of_blocks_homed_to_this_method(ho->get_count_of_blocks_homed_to_this_method_ctx() - 1);
  Oop me = as_oop();
  The_Memory_System()->object_table->set_dbg_y(me, 0);
  The_Memory_System()->object_table->set_dbg_z(me, 0);
# endif
}


inline Object* Object::get_orig_block_home() {
# if Extra_OTE_Words_for_Debugging_Block_Context_Method_Change_Bug
  return (Object*) The_Memory_System()->object_table->get_dbg_z(as_oop());
# else
  return NULL;
# endif
}



inline Oop Object::get_original_block_IP() {
# if Extra_OTE_Words_for_Debugging_Block_Context_Method_Change_Bug
  return Oop::from_bits(The_Memory_System()->object_table->get_dbg_t(as_oop()));
# else
  return Oop::from_int(-17);
# endif
}


inline void Object::set_count_of_blocks_homed_to_this_method(oop_int_t x) {
# if Extra_OTE_Words_for_Debugging_Block_Context_Method_Change_Bug
  The_Memory_System()->object_table->set_dbg_t(as_oop(), x);
# endif
}

inline oop_int_t Object::get_count_of_blocks_homed_to_this_method_ctx() {
# if Extra_OTE_Words_for_Debugging_Block_Context_Method_Change_Bug
  return The_Memory_System()->object_table->get_dbg_t(as_oop());
# else
  return 0;
# endif
}

inline bool Object::image_is_pre_4_1() { 
  return The_Squeak_Interpreter()->image_version == Squeak_Image_Reader::Pre_Closure_32_Bit_Image_Version; 
}
