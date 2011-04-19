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


bool Object::verify() {
  verify_address();
  if (isFreeObject()) {
    return true;
  }
  assert_always(sizeBits() >= sizeof(baseHeader));
  verify_preheader();
  FOR_EACH_OOP_IN_OBJECT_EXCEPT_CLASS(this, oop_ptr)
    oop_ptr->verify_oop();
  if ( contains_class_and_type_word() )
    get_class_oop().verify_oop();
  return okayOop();
}

bool Object::verify_address() {
  assert_always(my_heap_contains_me() );
  return true;
}


bool Object::verify_preheader() {
  return verify_extra_preheader_word() && verify_backpointer();
}


bool Object::verify_backpointer() {
  Oop x = backpointer();
  assert_always_msg(   x.as_object_unchecked() == this, "bad backpointer");
  return true;
}

bool Object::verify_extra_preheader_word() {
  return  !Extra_Preheader_Word_Experiment || Oop::from_bits(get_extra_preheader_word()).verify_oop();
}


void Object::dp() { print(stderr_printer); stderr_printer->nl(); }

void Object::print(Printer* p) {
  if (!my_heap_contains_me()) {
    p->printf("Wild pointer; object not in any heap");
    return;
  }
  if (this == NULL) {
    p->printf("NULL");
    return;
  }
  Oop klass = fetchClass();
  bool is_meta;
  Oop className = klass.as_object()->name_of_class_or_metaclass(&is_meta);
  Object_p class_name_obj = className.as_object();
  Oop n;

  if (class_name_obj->isBytes() && !class_name_obj->isCompiledMethod())
    ;
  else {
    p->printf("bad class");
    return;
  }
  assert(class_name_obj->isBytes() && !class_name_obj->isCompiledMethod());

  if (
      (class_name_obj->equals_string("ByteSymbol")
       || class_name_obj->equals_string("ByteString"))
      && !is_meta) {
    p->printf("#");
    print_bytes(p);
  }
  else if (class_name_obj->equals_string("Float") && !is_meta) {
    p->printf("%f(float)", fetchFloatAtinto());
  }
  else {
    p->printf("%s ", is_meta ? "class" :  is_vowel(class_name_obj->fetchByte(0)) ? "an" : "a" );
    class_name_obj->print_bytes(p);
    if (isBytes() && !isCompiledMethod()) {
       p->printf("  ");
      if (klass == The_Squeak_Interpreter()->splObj((Special_Indices::ClassByteArray)))
        print_bytes_in_array(p);
      else
        print_bytes(p);
    }
  }
}


void Object::print_class(Printer* p) {
  if (this == NULL) {
    p->printf("NULL");
    return;
  }
  bool is_meta;
  Oop className = name_of_class_or_metaclass(&is_meta);
  Object_p class_name_obj = className.as_object();
  Oop n;

  if (class_name_obj->isBytes() && !class_name_obj->isCompiledMethod())
    ;
  else {
    p->printf("bad class");
    return;
  }
  assert(class_name_obj->isBytes() && !class_name_obj->isCompiledMethod());


  p->printf("%s", is_meta ? "class " : "");
  class_name_obj->print_bytes(p);
}

void Object::print_bytes(Printer* p) {
  p->printf("%.*s", lengthOf(), first_byte_address());
}

void Object::print_bytes_in_array(Printer* p ) {
  p->dittoing_off();
  for (u_int32 i = 0;  i < lengthOf();  ++i)
    p->printf("0x%x ", (u_char)(first_byte_address()[i]));
  p->dittoing_on();
}


void Object::print_compiled_method(Printer* p) {
  if (!isCompiledMethod()) fatal("not a method");
  for (int i = 0;  i < literalCount();  ++i) {
    p->printf("literal %d: ", i);  literal(i).print(p); p->nl();
  }
  for (int i = 0;  i < byteSize() - sizeof(oop_int_t);  ++i) {
    u_char* bcp = (u_char*)&first_byte_address()[i];
    u_char bc = *bcp;
    p->printf("0x%x: byte %d: %d %s\n", bcp, i,  bc, Squeak_Interpreter::bytecode_name(bc));
  }
  p->nl();
}


Oop Object::className() {
  return fetchWordLength() <= Object_Indices::Class_Name_Index
    ?  The_Squeak_Interpreter()->roots.nilObj
    :  fetchPointer(Object_Indices::Class_Name_Index);
}

Oop Object::name_of_class_or_metaclass(bool* is_meta) {
  Oop cn = className();
  if (!cn.is_mem()  ||  !The_Memory_System()->object_table->probably_contains((void*)cn.bits())  ||  !The_Memory_System()->contains(cn.as_object()))
    return The_Squeak_Interpreter()->roots.nilObj;
  return (*is_meta = !(cn.bits() != 0  && cn.isBytes()))
    ? fetchPointer(Object_Indices::This_Class_Index).as_object()->className()
    : cn;
}

Oop Object::positive32BitIntegerFor(u_int32 integerValue) {
  // "Note - integerValue is interpreted as POSITIVE, eg, as the result of Bitmap>at:, or integer>bitAnd:."
  if ((int)integerValue >= 0  &&  Oop::isIntegerValue(integerValue))
    return Oop::from_int(integerValue);
  Object_p clpi = The_Squeak_Interpreter()->splObj_obj(Special_Indices::ClassLargePositiveInteger);
  Object_p newLargeInteger =
    bytesPerWord == 4
    // "Faster instantiateSmallClass: currently only works with integral word size."
    ? clpi->instantiateSmallClass(BaseHeaderSize + sizeof(int32))
      // "Cant use instantiateSmallClass: due to integral word requirement."
    : clpi->instantiateClass(sizeof(int32));

  newLargeInteger->storeByte( 3, u_char(integerValue >> 24)  &  '\xff');
  newLargeInteger->storeByte( 2, u_char(integerValue >> 16)  &  '\xff');
  newLargeInteger->storeByte( 1, u_char(integerValue >>  8)  &  '\xff');
  newLargeInteger->storeByte( 0, u_char(integerValue      )  &  '\xff');

  return newLargeInteger->as_oop();
}


Oop Object::signed32BitIntegerFor(int32 integerValue) {
  if (Oop::isIntegerValue(integerValue))
    return Oop::from_int(integerValue);
  int32 v = integerValue < 0 ? -integerValue : integerValue;
  Object_p klass =
    integerValue < 0
    ?  The_Squeak_Interpreter()->splObj_obj(Special_Indices::ClassLargeNegativeInteger)
    :  The_Squeak_Interpreter()->splObj_obj(Special_Indices::ClassLargePositiveInteger);
  Object_p newLargeInteger = klass->instantiateClass(sizeof(int32));

  newLargeInteger->storeByte( 3, u_char(v >> 24)  &  '\xff');
  newLargeInteger->storeByte( 2, u_char(v >> 16)  &  '\xff');
  newLargeInteger->storeByte( 1, u_char(v >>  8)  &  '\xff');
  newLargeInteger->storeByte( 0, u_char(v      )  &  '\xff');

  return newLargeInteger->as_oop();
}


Oop Object::positive64BitIntegerFor(u_int64 integerValue) {
  // "Note - integerValue is interpreted as POSITIVE, eg, as the result of Bitmap>at:, or integer>bitAnd:."
  if (Oop::isIntegerValue(integerValue))
    return Object::positive32BitIntegerFor(int32(integerValue));

  Object_p clpi = The_Squeak_Interpreter()->splObj_obj(Special_Indices::ClassLargePositiveInteger);
  Object_p newLargeInteger = clpi->instantiateClass(sizeof(int64));

  newLargeInteger->storeByte( 7, u_char(integerValue >> 56LL)  &  '\xff');
  newLargeInteger->storeByte( 6, u_char(integerValue >> 48LL)  &  '\xff');
  newLargeInteger->storeByte( 5, u_char(integerValue >> 40LL)  &  '\xff');
  newLargeInteger->storeByte( 4, u_char(integerValue >> 32LL)  &  '\xff');
  newLargeInteger->storeByte( 3, u_char(integerValue >> 24LL)  &  '\xff');
  newLargeInteger->storeByte( 2, u_char(integerValue >> 16LL)  &  '\xff');
  newLargeInteger->storeByte( 1, u_char(integerValue >>  8LL)  &  '\xff');
  newLargeInteger->storeByte( 0, u_char(integerValue       )  &  '\xff');

  return newLargeInteger->as_oop();
}


Oop Object::signed64BitIntegerFor(int64 integerValue) {
  if (Oop::isIntegerValue(integerValue))
    return Oop::from_int(integerValue);
  if (int64(int32(integerValue)) == integerValue)
    return signed32BitIntegerFor(int32(integerValue));

  int64 v = integerValue < 0LL ? -integerValue : integerValue;
  Object_p klass =
  integerValue < 0LL
  ?  The_Squeak_Interpreter()->splObj_obj(Special_Indices::ClassLargeNegativeInteger)
  :  The_Squeak_Interpreter()->splObj_obj(Special_Indices::ClassLargePositiveInteger);
  Object_p newLargeInteger = klass->instantiateClass(sizeof(int64));

  newLargeInteger->storeByte( 7, u_char(v >> 56LL)  &  '\xff');
  newLargeInteger->storeByte( 6, u_char(v >> 48LL)  &  '\xff');
  newLargeInteger->storeByte( 5, u_char(v >> 40LL)  &  '\xff');
  newLargeInteger->storeByte( 4, u_char(v >> 32LL)  &  '\xff');
  newLargeInteger->storeByte( 3, u_char(v >> 24LL)  &  '\xff');
  newLargeInteger->storeByte( 2, u_char(v >> 16LL)  &  '\xff');
  newLargeInteger->storeByte( 1, u_char(v >>  8LL)  &  '\xff');
  newLargeInteger->storeByte( 0, u_char(v      )   &  '\xff');

  return newLargeInteger->as_oop();
}




Object_p Object::instantiateClass(oop_int_t size, Logical_Core* where) {
    /*
     "NOTE: This method supports the backward-compatible split instSize field of the
     class format word. The sizeHiBits will go away and other shifts change by 2
     when the split fields get merged in an (incompatible) image change."
     */
  assert(size >= 0); // size is indexable field count

  // xxxxxx do it on that core instead of remotely? -- dmu
  // Explanation: This code allocates an object onto another core's heap (if where is passed in).
  // It might be faster to ask the core that owns the heap to do the allocation, rather than may the
  // memory penalty. -- dmu 4/09

  Multicore_Object_Heap* h =  The_Memory_System()->heaps[where == NULL  ?  Logical_Core::my_rank()  :  where->rank()][Memory_System::read_write];

  int hash = h->newObjectHash();
  oop_int_t classFormat = formatOfClass();
  // low 2 bits are 0
  oop_int_t header1 = (classFormat & 0x1ff00) | (hash << HashShift) & HashMask;
  Oop header2 = as_oop();
  oop_int_t header3 = 0;
  oop_int_t cClass = header1 & CompactClassMask;
  oop_int_t sizeHiBits = (classFormat & 0x60000) >> 9;
  int byteSize = (classFormat & (SizeMask | Size4Bit)) | sizeHiBits; // low bits 0
  int format = (classFormat >> 8) & 15;

  if (!Format::has_bytes(format)) {
    if (format == Format::indexable_word_fields_only) {
      // long32 bitmaps
      byteSize += size * sizeof(int32);
      byteSize = round_up_by_power_of_two(byteSize, sizeof(int32));
      // XXX if 64bit, must do more here, see ST
    }
    else
      byteSize += size * sizeof(int32); // Arrays and 64-bit bitmaps
  }
  else {
    // strings and methods
    byteSize += round_up_by_power_of_two(size, sizeof(int32));
    // redo next bit for 64 XXX
    // size info goes in format field
    // how many bytes true size is less than format
    // see lengthOf
    header1 |= ((byteSize - size)  &  (sizeof(int32)-1))   <<   FormatShift;
  }
  if (byteSize > 255) // need size header
    header3 = byteSize;
  else
    header1 |= byteSize;
  int hdrSize = header3 > 0  ?  3  :    cClass == 0  ?  2  :  1;

  return h->allocate(byteSize, hdrSize, header1, header2, header3, true, Format::has_only_oops(format));
}


bool Object::okayOop() {
  // "Verify that the given oop is legitimate. Check address, header, and size but not class."

  // address and size checks
  Abstract_Object_Heap* h = my_heap();
  bool ok = h->contains(this)  &&  h->contains(&as_char_p()[sizeBits()-1]);

  assert_always_msg( ok, "oop not in heap or size would extend beyond end of memory");

  // header type checks
  switch(headerType()) {
      default: fatal("illegal header type");
      case Header_Type::Free:  fatal("oop is a free chunk, not an object");

      case Header_Type::Short:
        if ( compact_class_index() == 0 ) {
            lprintf("found zero compact class field in short header: Oop 0x%x, Object_p 0x%x\n",
                    as_oop().bits(), this);
            fatal("cannot have zero compact class field in a short header");
        }
        break;

    case Header_Type::SizeAndClass:
        assert_always_msg( h->contains(&sizeHeader()),
                          "size header is before start");
        assert_always_msg( Header_Type::extract_from(sizeHeader()) == headerType(), "size header has wrong type");

      // fall through
      case Header_Type::Class:
        assert_always_msg(  h->contains(&class_and_type_word()), "class header word is before start");
        assert_always_msg( Header_Type::extract_from(class_and_type_word()) == headerType(), "class header word has wrong type" );
        break;
  }

	// "format check"
  assert_always_msg( Format::is_valid(format()), "oop has unknown format type");

	/*
   "mark and root bit checks"
	unusedBit := 16r20000000.
	bytesPerWord = 8
ifTrue:
  [unusedBit := unusedBit << 16.
   unusedBit := unusedBit << 16].
	((self longAt: oop) bitAnd: unusedBit) = 0
ifFalse: [ self error: 'unused header bit 30 is set; should be zero' ].
  "xxx_dmu
	((self longAt: oop) bitAnd: MarkBit) = 0
  ifFalse: [ self error: 'mark bit should not be set except during GC' ].
  xxx_dmu"
	(((self longAt: oop) bitAnd: RootBit) = 1 and:
	 [oop >= youngStart])
ifTrue: [ self error: 'root bit is set in a young object' ].
   */
   return true;
}

bool Object::hasOkayClass() {
  // "Attempt to verify that the given oop has a reasonable behavior. The class must be a valid, non-integer oop and must not be nilObj. It must be a pointers object with three or more fields. Finally, the instance specification field of the behavior must match that of the instance."
  okayOop();
  Oop klass_oop = fetchClass();
  assert_always_msg(klass_oop.is_mem(), "a SmallInteger is not a valid class or behavior");
  Object_p klass = klass_oop.as_object();
  klass->okayOop();
  assert_always_msg(klass->isPointers()  &&  klass->lengthOf() >= 3,
                    "a class (behavior) must be a pointers object of size >= 3");
  oop_int_t formatMask = isBytes() ? 0xc00 /* ignore extra bytes size bits */ : 0xf00;
  oop_int_t behaviorFormatBits = klass->formatOfClass() & formatMask;
  oop_int_t oopFormatBits = baseHeader & formatMask;
  assert_always_msg(behaviorFormatBits == oopFormatBits,
                    "object and its class (behavior) formats differ'");
  return true;
}




// ObjectMemory object enumeration

Oop* Object::last_pointer_addr() {
  return (Oop*)&as_char_p()[lastPointer()];
}

Oop* Object::last_strong_pointer_addr() {
  return &as_oop_p()[nonWeakFieldsOf()];
}

Oop* Object::last_strong_pointer_addr_remembering_weak_roots(Abstract_Mark_Sweep_Collector *gc) {
  return  isWeak()  &&  gc->add_weakRoot(as_oop())  ?  last_strong_pointer_addr()  :  last_pointer_addr();
}

// ObjectMemory intialization


void Object::do_all_oops_of_object(Oop_Closure* oc, bool do_checks) {
  if (isFreeObject())
    return;
  FOR_EACH_OOP_IN_OBJECT_EXCEPT_CLASS(this, oopp) {
    if (do_checks)
      my_heap()->contains(oopp);
    oc->value(oopp, (Object_p)this);
  }
  if (contains_class_and_type_word()) {
    Oop c = get_class_oop();
    Oop new_c = c;
    oc->value(&new_c, (Object_p)this);
    if (new_c != c)
      set_class_oop(new_c);
  }
  if (Extra_Preheader_Word_Experiment)
    oc->value((Oop*)extra_preheader_word(), (Object_p)this);
}


void Object::do_all_oops_of_object_for_reading_snapshot(Squeak_Image_Reader* r) {
  if (isFreeObject())
    return;
  FOR_EACH_OOP_IN_OBJECT_EXCEPT_CLASS(this, oopp) {
    Oop x = *oopp;
    if (x.is_mem()) {
      Oop contents = r->oop_for_oop(x);
      IMAGE_READING_DEBUG_STORE_CHECK(oopp, contents); 
      *oopp = contents;
    }
  }
  if (contains_class_and_type_word()) {
    Oop c = get_class_oop();
    set_class_oop_no_barrier(r->oop_for_oop(c));
  }
}


void Object::do_all_oops_of_object_for_marking(Abstract_Mark_Sweep_Collector* gc, bool do_checks) {
  FOR_EACH_STRONG_OOP_IN_OBJECT_EXCEPT_CLASS_RECORDING_WEAK_ROOTS(this, oopp, gc) {
    if (do_checks)
      my_heap()->contains(oopp);
    gc->mark(oopp);
  }
  if (contains_class_and_type_word()) {
    Oop c = get_class_oop();
    gc->mark(&c);
  }
  if (Extra_Preheader_Word_Experiment)
    gc->mark((Oop*)extra_preheader_word());
}




// ObjectMemory allocation
Oop Object::clone() {
  // Return a shallow copy, may GC
  int extraHdrBytes = extra_header_bytes();
  int32 bytes = sizeBits() + extraHdrBytes;

  // alloc space, remap in case of GC
  The_Squeak_Interpreter()->pushRemappableOop(as_oop());
  // is it safe?
  Logical_Core* c = The_Memory_System()->coreWithSufficientSpaceToAllocate( 2500 + bytes, Memory_System::read_write);
  if ( c == NULL) {
    The_Squeak_Interpreter()->popRemappableOop();
    return Oop::from_int(0);
  }
  Multicore_Object_Heap* h = The_Memory_System()->heaps[c->rank()][Memory_System::read_write];
  
  // Follow the pattern in Multicore_Object_Heap::allocate -- dmu & sm:
  
  Oop* newChunk = (Oop*)h->allocateChunk_for_a_new_object_and_safepoint_if_needed(bytes);
  Safepoint_Ability sa(false); // from here on, no GCs!


  Oop remappedOop = The_Squeak_Interpreter()->popRemappableOop();
  Object_p remappedObject = remappedOop.as_object(); // GC may have moved it; cannot use THIS in rest of method
  Object_p newObj = (Object_p)(Object*) ((char*)newChunk + extraHdrBytes);

  // copy old to new incl all header words, fix backpointer later, might include extra header words
  The_Memory_System()->store_bytes_enforcing_coherence(
                               newChunk, // dst
                               remappedObject->my_chunk(), // src
                               bytes,
                               newObj); // n bytes

  // fix base header: compute new hash and clear Mark and Root bits
  oop_int_t hash = h->newObjectHash(); // even though newChunk may be in global heap
  The_Memory_System()->store_enforcing_coherence(&newObj->baseHeader,
                                              newObj->baseHeader & (Header_Type::Mask | SizeMask | CompactClassMask | FormatMask)
                                              |   (hash << HashShift) & HashMask,
                                              newObj);

  The_Memory_System()->object_table->allocate_oop_and_set_preheader(newObj, Logical_Core::my_rank()  COMMA_TRUE_OR_NOTHING);
  
# if Extra_Preheader_Word_Experiment
  oop_int_t ew = remappedObject->get_extra_preheader_word();
  assert_always(ew && (!Oop::from_bits(ew).is_int()  ||  ew == Oop::from_int(0).bits())); // bug hunt
  newObj->set_extra_preheader_word(ew);
# endif
  
  // newObj->beRootIfOld();
  
  return newObj->as_oop();
}


Object_p Object::process_list_for_priority_of_process() {
  int priority = priority_of_process();
  Object_p processLists = The_Squeak_Interpreter()->process_lists_of_scheduler();
  assert(priority - 1  <=  processLists->fetchWordLength());
  return processLists->fetchPointer(priority - 1).as_object();
}

// Save on given list for priority
void Object::add_process_to_scheduler_list() {
  process_list_for_priority_of_process()->addLastLinkToList(as_oop());
}


Oop Object::get_suspended_context_of_process_and_mark_running() {
  assert(Scheduler_Mutex::is_held());
  The_Squeak_Interpreter()->assert_registers_stored();
  Oop ctx = fetchPointer(Object_Indices::SuspendedContextIndex);
  assert(ctx != The_Squeak_Interpreter()->roots.nilObj);
  if (Print_Scheduler_Verbose) {
    debug_printer->printf("scheduler: on %d get_suspended_context_of_process_and_mark_running ", Logical_Core::my_rank());
    this->print_process_or_nil(debug_printer);
    debug_printer->printf(" to nil");
    debug_printer->nl();
  }

  storePointer(Object_Indices::SuspendedContextIndex, The_Squeak_Interpreter()->roots.nilObj);
  store_host_core_of_process(Logical_Core::my_rank());
  return ctx;
}

void Object::set_suspended_context_of_process(Oop ctx) {
  assert(Scheduler_Mutex::is_held());
  assert(ctx != fetchPointer(Object_Indices::SuspendedContextIndex));
  assert(is_process_running());
  The_Squeak_Interpreter()->assert_registers_stored();
  if (Print_Scheduler_Verbose) {
    debug_printer->printf("scheduler: on %d set_suspended_context_of_process ", Logical_Core::my_rank());
    this->print_process_or_nil(debug_printer);
    debug_printer->printf(" to ");
    ctx.print(debug_printer);
    debug_printer->nl();
  }
  storePointer(Object_Indices::SuspendedContextIndex, ctx);
  store_host_core_of_process(-1);
}

int Object::priority_of_process_or_nil() {
  return (this == (Object*)The_Squeak_Interpreter()->roots.nilObj.as_object()) ? -1 : priority_of_process();
}

bool Object::is_process_running() {
  return fetchPointer(Object_Indices::SuspendedContextIndex) == The_Squeak_Interpreter()->roots.nilObj;
}




bool Object::is_process_allowed_to_run_on_this_core() {
  int acm = The_Process_Field_Locator.index_of_process_inst_var(Process_Field_Locator::coreMask);
  if (acm < 0) return true;

  The_Squeak_Interpreter()->successFlag = true;
  u_int64 mask = The_Squeak_Interpreter()->positive64BitValueOf(fetchPointer(acm));
  if (!The_Squeak_Interpreter()->successFlag) {
    The_Squeak_Interpreter()->successFlag = true;
    return true;
  }
  
  bool r =  ((1LL << Logical_Core::my_rank()) & mask) ? true : false;
  return r;
}

void Object::store_host_core_of_process(int r) {
  int hc = The_Process_Field_Locator.index_of_process_inst_var(Process_Field_Locator::hostCore);
  if (hc < 0) return;
  storeIntegerUnchecked(hc, r);
}

void Object::store_allowable_cores_of_process(u_int64 bitMask) {
  int acm = The_Process_Field_Locator.index_of_process_inst_var(Process_Field_Locator::coreMask);
  if (acm < 0) return;
  storePointer(acm, positive64BitIntegerFor(bitMask));
}

void Object::kvetch_nil_list_of_process(const char* why) {
  if (my_list_of_process() == The_Squeak_Interpreter()->roots.nilObj ) {
    static int kvetch_count = 10;
    if (kvetch_count) {
      lprintf("WARNING: image has not been modified to allow dynamic priority changes; cannot nil out myList before calling remove_process_from_scheduler_list(%s)\n", why);
      --kvetch_count;
    }
  }
}

// Returns list if any process was on

Oop Object::remove_process_from_scheduler_list(const char* why) {
  Scheduler_Mutex sm("remove_process_from_scheduler_list");

  Oop processListOop = my_list_of_process();
  Object_p processList =  
    processListOop == The_Squeak_Interpreter()->roots.nilObj  
      ? process_list_for_priority_of_process() 
      : processListOop.as_object();

  if (processList->isEmptyList()) {
    static const bool tolerate_stock_images = true; // Running process not in scheduler list
    if (tolerate_stock_images  &&  Logical_Core::group_size == 1  &&  !The_Squeak_Interpreter()->primitiveThisProcess_was_called())
      return The_Squeak_Interpreter()->roots.nilObj;
    // Next statement may fail if halfway through converting image for RVM -- dmu 6/10
    // if (check_assertions) { lprintf("not in empty list: %s\n", why); fatal("not in list"); }
    return The_Squeak_Interpreter()->roots.nilObj;
  }
  Oop  first_proc = processList->fetchPointer(Object_Indices::FirstLinkIndex);
  Oop   last_proc = processList->fetchPointer(Object_Indices:: LastLinkIndex);

  Oop        proc = first_proc;
  Object_p proc_obj = proc.as_object();
  Object_p prior_proc_obj  = (Object_p)NULL;

  while (proc_obj != this) {
    prior_proc_obj = proc_obj;
    proc = proc_obj->fetchPointer(Object_Indices::NextLinkIndex);
    if (proc == The_Squeak_Interpreter()->roots.nilObj) {
      kvetch_nil_list_of_process(why);
      // normal with suspend change lprintf( "WARNING: process not found in list");
      nil_out_my_list_and_next_link_fields_of_process();
      return The_Squeak_Interpreter()->roots.nilObj;
    }
    proc_obj = proc.as_object();
  }
  
  kvetch_nil_list_of_process(why);

  if (first_proc == proc)
    processList->removeFirstLinkOfList();

  else if (last_proc == proc)
    processList->removeLastLinkOfList(prior_proc_obj);

  else
    processList->removeMiddleLinkOfList(prior_proc_obj, proc_obj);
  
  return processList->as_oop();
}


Oop Object::removeFirstLinkOfList() {
  // rm first process
  const int fl = Object_Indices::FirstLinkIndex;
  const int ll = Object_Indices:: LastLinkIndex;
  const int nl = Object_Indices:: NextLinkIndex;
  Oop nil = The_Squeak_Interpreter()->roots.nilObj;

  Oop first = fetchPointer(fl);  Oop last  = fetchPointer(ll);
  Object_p fo = first.as_object();
  if (first == last) {
    storePointer(fl, nil);
    storePointer(ll, nil);
  }
  else
    storePointer(fl, fo->fetchPointer(nl));
  fo->nil_out_my_list_and_next_link_fields_of_process();
  return first;
}

Oop  Object::removeLastLinkOfList(Object_p penultimate) {
  const int nl = Object_Indices::NextLinkIndex;
  Oop nil = The_Squeak_Interpreter()->roots.nilObj;
  Oop last = fetchPointer(Object_Indices::LastLinkIndex);
  storePointer(Object_Indices::LastLinkIndex, penultimate->as_oop());
  penultimate->storePointer(nl, nil);
  assert(last.as_object()->fetchPointer(nl) == nil);
  last.as_object()->nil_out_my_list_and_next_link_fields_of_process();
  return last;
}

Oop Object::removeMiddleLinkOfList(Object_p prior, Object_p mid) {
  const int nl = Object_Indices::NextLinkIndex;
  prior->storePointer(nl, mid->fetchPointer(nl));
  mid->nil_out_my_list_and_next_link_fields_of_process();
  return mid->as_oop();
}

void Object::addLastLinkToList(Oop aProcess) {
  // Add given proc to linked list receiver, set backpointer
  if (isEmptyList())
    storePointer(Object_Indices::FirstLinkIndex, aProcess);
  else
    fetchPointer(Object_Indices::LastLinkIndex).as_object()->storePointer(Object_Indices::NextLinkIndex, aProcess);

  storePointer(Object_Indices::LastLinkIndex, aProcess);
  aProcess.as_object()->storePointer(Object_Indices::MyListIndex, as_oop());
}


void Object::nil_out_my_list_and_next_link_fields_of_process() {
  Oop nil = The_Squeak_Interpreter()->roots.nilObj;
  storePointer(Object_Indices::  MyListIndex, nil);
  storePointer(Object_Indices::NextLinkIndex, nil);
}

bool Object::isEmptyList() {
  Oop first = fetchPointer(Object_Indices::FirstLinkIndex);
  assert(first.bits() != 0);
  return first == The_Squeak_Interpreter()->roots.nilObj;
}


void Object::print_process_or_nil(Printer* p, bool print_stack) {
  Oop my_oop = as_oop();
  if (my_oop == The_Squeak_Interpreter()->roots.nilObj) {
    p->printf("nil");
    return;
  }
  print(p);
  
  p->printf("(0x%x, hash %d, pri %d, %s, ", as_oop().bits(), hashBits(), priority_of_process(), is_process_running() ? "running" : "not running");

  if (Print_Scheduler_Verbose) {
    p->printf("myList: ");    my_list_of_process().print(p);  p->printf(", ");
    p->printf("nextLink: ");  fetchPointer(Object_Indices::NextLinkIndex).print(p);  p->printf(", ");
  }
  
  Oop name = name_of_process();
  if (name != The_Squeak_Interpreter()->roots.nilObj) {
    p->printf("name: "); name.as_object()->print_bytes(p); p->printf("0x%x  ", name.as_object()->first_byte_address());
    //if (strncasecmp(name.as_object()->first_byte_address(), "ScreenController", 16) == 0) {
    //  print_stack = true;
    //}    
  }

  int core = Object::core_where_process_is_running();

  if (core != -1)  p->printf("running on %d, ", core);
  p->printf(")");
  
  if (print_stack) {
    p->nl();
    The_Squeak_Interpreter()->print_stack_trace(p, (Object_p)this);
    p->nl();
  }
}


bool Object::verify_process() {
  assert_always(my_list_of_process().is_mem());
  Oop& p = pointer_at(Object_Indices::NextLinkIndex);
  assert_always(p.is_mem());
  return true;
}


void Object::print_frame(Printer* p) {
  Oop sender = fetchPointer(Object_Indices::SenderIndex);
  int sp = fetchInteger(Object_Indices::StackPointerIndex);

  Object_p home = home_of_block_or_method_context();
  Oop method = home->fetchPointer(Object_Indices::MethodIndex);
  bool is_block = is_this_context_a_block_context();
  if (method.bits() == 0) {
    p->printf("method oop is zero\n");
    return;
  }
  Object_p mo = method.is_mem() ? method.as_object() : (Object_p)NULL;

  Oop rcvr = home->fetchPointer(Object_Indices::ReceiverIndex);
  Object_p klass = rcvr.fetchClass().as_object();


  Oop* stack = &as_oop_p()[Object_Indices::ContextFixedSizePlusHeader];
  int ip = mo == NULL ? -17 :
    fetchInteger(Object_Indices::InstructionPointerIndex)
      - ((Object_Indices::LiteralStart + mo->literalCount() * bytesPerWord) + 1);

  p->printf("0x%x, ip %3d, sp %2d:  ", this, ip, sp);


  Oop sel, mclass;
  bool have_sel_and_mclass = klass->selector_and_class_of_method_in_me_or_ancestors(method, &sel, &mclass);

  if (have_sel_and_mclass) {
    if (mclass == The_Squeak_Interpreter()->roots.nilObj)
      mclass = rcvr.fetchClass();

    p->printf(" "); rcvr.print(p);
    if (rcvr.is_mem()) p->printf("<0x%x>", rcvr.as_untracked_object_ptr());
    if (mclass.as_object() != klass) {
      p->printf("(");
      bool is_meta;
      Oop mclassName = mclass.as_object()->name_of_class_or_metaclass(&is_meta);
      if (is_meta) p->printf("class ");
      mclassName.as_object()->print_bytes(p);
      p->printf(")");
    }
    p->printf(" ");

    p->printf(">>   ");
    if (sel.is_int()) p->printf("bad sel");
    else if (sel != The_Squeak_Interpreter()->roots.nilObj) {
      sel.as_object()->print_bytes(p);
    }
  }
  p->printf(is_block ? " [] " : "    ");

  p->printf("  ");
  for (int i = 0; i < sp;  ++i) {
    stack[i].print(p);
    if (i < sp-1) p->printf(", ");
  }
  p->nl();
  // int blockArgCount = method.integerValue();
}


bool Object::selector_and_class_of_method_in_me_or_ancestors(Oop method, Oop* selp, Oop* classp) {
  if (!method.is_mem())  {
    lprintf( "selector_and_class_of_method_in_me_or_ancestors: method 0x%x is an int\n", method.bits());
    return false;
  }
  Object_p mo = method.as_object();
  if (!The_Memory_System()->contains(mo)) {
    lprintf( "selector_and_class_of_method_in_me_or_ancestors: method 0x%x is not in heap\n", (Object*)mo);
    return false;
  }
  Oop    methodDictOop = fetchPointer(Object_Indices::MessageDictionaryIndex);
  if (!methodDictOop.is_mem()) {
    lprintf( "selector_and_class_of_method_in_me_or_ancestors: methodDictOop 0x%x is an int\n", methodDictOop.bits());
    return false;
  }
  Object_p methodDict = methodDictOop.as_object();
  if (!The_Memory_System()->contains(methodDict)) {
    lprintf( "selector_and_class_of_method_in_me_or_ancestors: methodDict 0x%x is not in heap\n", (Object*)methodDict);
    return false;
  }
  Oop sel = methodDict->key_at_identity_value(method);
  if (sel.bits() != Oop::Illegals::uninitialized) {
    *selp = sel;
    *classp = as_oop();
    return true;
  }
  Oop superclass = fetchPointer(Object_Indices::SuperclassIndex);
  if (superclass == The_Squeak_Interpreter()->roots.nilObj) {
    static bool warned = false; // Stefan: is set only once, thus it is threadsafe
    if (!warned) {
      warned = true;
      lprintf( "selector_and_class_of_method_in_me_or_ancestors: did not find method 0x%x in class 0x%x\n",
              (Object*)mo,  this);
    }
    return false;
  }
  return superclass.as_object()->selector_and_class_of_method_in_me_or_ancestors(method, selp, classp);
}


Oop Object::key_at_identity_value(Oop val) {
  if (!isPointers())
    lprintf("key_at_identity_value: this is pointers 0x%x\n", this);

  else {

    Oop methodsOop = fetchPointer(Object_Indices::MethodArrayIndex);
    if (!methodsOop.isPointers())
      lprintf("key_at_identity_value: methodsOop is not pointers 0x%x\n", methodsOop.bits());

    else {
      Object_p methods = methodsOop.as_object();
      int length = fetchWordLength() - Object_Indices::SelectorStart;
      for (int index = 0;  index < length;  ++index)
        if (methods->fetchPointer(index) == val)
          return fetchPointer(index + Object_Indices::SelectorStart);
    }
  }

  return Oop::from_bits(Oop::Illegals::uninitialized);
}



Object_p Object::makePoint(oop_int_t x, oop_int_t y) {
  Object_p pt = The_Squeak_Interpreter()->splObj_obj(Special_Indices::ClassPoint)->instantiateSmallClass(3 * bytesPerWord);
  pt->storeIntegerUnchecked(Object_Indices::XIndex, x);
  pt->storeIntegerUnchecked(Object_Indices::YIndex, y);
  return pt;
}

Object_p Object::makeString(const char* str) {
  return makeString(str, strlen(str));
}

Object_p Object::makeString(const char* str, int n) {
  Object_p r = The_Squeak_Interpreter()->classString()->instantiateClass(n);
  The_Memory_System()->store_bytes_enforcing_coherence(r->as_char_p() + Object::BaseHeaderSize, str, n, r); // dst src n
  return r;
}


void Object::move_to_heap(int r, int rw_or_rm, bool do_sync) {
  if (The_Memory_System()->rank_for_address(this) == r
  &&  The_Memory_System()->mutability_for_address(this) == rw_or_rm)
    return;

  int ehb = extra_header_bytes();
  int bnc = bytes_to_next_chunk();
  Multicore_Object_Heap* h = The_Memory_System()->heaps[r][rw_or_rm];

  assert_always(rw_or_rm == Memory_System::read_write  ||  is_suitable_for_replication());

  Safepoint_for_moving_objects sf("move_to_heap");
  Safepoint_Ability sa(false);

  Oop oop = as_oop();
  if (do_sync) {
    The_Squeak_Interpreter()->preGCAction_everywhere(false);  // false because caches are oop-based, and we just move objs
    flushFreeContextsMessage_class().send_to_all_cores(); // might move a free context, then it would not be in right place
  }


  The_Squeak_Interpreter()->pushRemappableOop(oop);

  Chunk* dst_chunk = h->allocateChunk(ehb + bnc);
  oop = The_Squeak_Interpreter()->popRemappableOop();
  char* src_chunk = as_char_p() - ehb;
  Object_p new_obj = (Object_p)(Object*) (((char*)dst_chunk) + ehb);

  h->enforce_coherence_before_store(dst_chunk, ehb + bnc);
  DEBUG_MULTIMOVE_CHECK(dst_chunk, src_chunk, (ehb + bnc) / bytes_per_oop );
  bcopy(src_chunk, dst_chunk, ehb + bnc);
  // set backpointer is redundant but this routine does the safepoint
  new_obj->set_object_address_and_backpointer(oop  COMMA_TRUE_OR_NOTHING);
  if (Extra_Preheader_Word_Experiment)
    new_obj->set_extra_preheader_word(get_extra_preheader_word());
  h->enforce_coherence_after_store(dst_chunk, ehb + bnc);

  ((Chunk*)src_chunk)->make_free_object(ehb + bnc, 2); // without this GC screws up

  if (do_sync) The_Squeak_Interpreter()->postGCAction_everywhere(false);
}


void Object::test() { assert_always(sizeof(Object) == 4); }

int Object::core_where_process_is_running() {
  if (!Track_Processes) fatal("Track_Processes must be set");
  Oop my_oop = as_oop();
  int running_where = -1;
  FOR_ALL_RANKS(i) {
    if (my_oop == The_Squeak_Interpreter()->running_process_by_core[i]) {
      if (running_where != -1) {
        error_printer->printf("ERROR: process pri %d running on two cores: %d and %d: ",
                              priority_of_process_or_nil(), running_where, i);

      }
      else
        running_where = i;
    }
  }
  return running_where;
}



// xxxxxx factor all places that IP is calculated someday -- dmu
// Explanation: The original interpreter does a lot of adding/subtracting to get the instruction pointer from the
// method address. I have partially but not completely factored this. -- dmu 4/09
u_char* Object::next_bc_to_execute_of_context() {
  /*
   "the instruction pointer is a pointer variable equal to
   method oop + ip + BaseHeaderSize
   -1 for 0-based addressing of fetchByte"
   */
  Object_p meth = home_of_block_or_method_context()->fetchPointer(Object_Indices::MethodIndex).as_object();
  oop_int_t ip_int = quickFetchInteger(Object_Indices::InstructionPointerIndex);
  return meth->as_u_char_p() + ip_int + Object::BaseHeaderSize - 1;
}


void Object::check_all_IPs_in_chain() {
  if (as_oop() == The_Squeak_Interpreter()->roots.nilObj) return;
  check_IP_in_context();
  fetchPointer(Object_Indices::SenderIndex).as_object()->check_all_IPs_in_chain();
}

void Object::check_IP_in_context() {
# if Extra_OTE_Words_for_Debugging_Block_Context_Method_Change_Bug
  Object* meth = home_of_block_or_method_context()->fetchPointer(Object_Indices::MethodIndex).as_object();
  u_char* my_ip = next_bc_to_execute_of_context();
  meth->check_IP_of_method(my_ip, this);
# endif
}


void Object::check_IP_of_method(u_char* bcp, Object_p ctx) {
  if (!isCompiledMethod()) fatal("check_method_is_correct: not a method");

  u_char* first_bc = (u_char*)first_byte_address();
  u_char* past_bc =  (u_char*)nextChunk();

  if (bcp < first_bc) {
    lprintf("bcp 0x%x < first_bc 0x%x:  past_bc 0x%x, method header 0x%x, at 0x%x, sizeBits: 0x%x, isBlock %d, ctx obj 0x%x\n",
            bcp, first_bc, past_bc, *as_oop_int_p(), this, sizeBits(), !ctx->isMethodContext(), (Object*)ctx);
    fatal("bcp < first_bc");
  }
  if (past_bc < bcp) {
    Oop orig_meth = ctx->get_orig_block_method();
    Oop curr_meth = as_oop();
    Object* orig_home = ctx->get_orig_block_home();
    Oop curr_home = ctx->fetchPointer(Object_Indices::HomeIndex);
    lprintf("past_bc 0x%x < bcp 0x%x:  first_bc 0x%x, method header 0x%x, at 0x%x, sizeBits: 00x%x, isBlock %d, ctx 0x%x, %s %s\n",
            past_bc, bcp, first_bc, *as_oop_int_p(), this, sizeBits(), !ctx->isMethodContext(), (Object*)ctx,
            orig_meth == curr_meth ? "method same" : "method changed",
            orig_home == curr_home.as_untracked_object_ptr() ? "home OBJ same" : "home OBJ changed");

    lprintf("home 0x%x, orig_method 0x%x, curr_method 0x%x\n",
            curr_home.bits(), orig_meth.bits(), curr_meth.bits());

    debug_printer->printf("orig_method: ");
    orig_meth.as_object()->print_compiled_method(debug_printer);
    debug_printer->nl();

    debug_printer->printf("curr_method: ");
    curr_meth.as_object()->print_compiled_method(debug_printer);
    debug_printer->nl();

    assert_always(curr_meth.as_object() == this);

    fatal("past_bc < bcp");
  }

}

void Object::cleanup_session_ID_and_ext_prim_index_of_external_primitive_literal() {
  storeIntegerUnchecked(Object_Indices::EPL_Session_ID,                                                           0);
  storeIntegerUnchecked(Object_Indices::EPL_External_Primitive_Table_Index, Abstract_Primitive_Table::lookup_needed);
}

Object_p Object::get_external_primitive_literal_of_method() {
  if (literalCount() <= Object_Indices::External_Primitive_Literal_Index)  return (Object_p)NULL;
  Oop lit = literal(Object_Indices::External_Primitive_Literal_Index);
  if (!lit.is_mem()) return (Object_p)NULL;
  Object_p lo = lit.as_object();
  return lo->isArray()  &&  lo->lengthOf() == Object_Indices::EPL_Length  ?  lo  :  (Object_p)NULL;
}


oop_int_t Object::nonWeakFieldsOf() {
  /*

  "Return the number of non-weak fields in oop (i.e. the number of fixed fields).
   Note: The following is copied from fixedFieldsOf:format:length: since we do know
   the format of the oop (e.g. format = 4) and thus don't need the length."
   | class classFormat |
   self inline: false. "No need to inline - we won't call this often"

   (self isWeakNonInt: oop) ifFalse:[self error:'Called fixedFieldsOfWeak: with a non-weak oop'].

   "fmt = 3 or 4: mixture of fixed and indexable fields, so must look at class format word"
   class := self fetchClassOf: oop.
   classFormat := self formatOfClass: class.
   ^ (classFormat >> 11 bitAnd: 16rC0) + (classFormat >> 2 bitAnd: 16r3F) - 1
*/

  assert_always(isWeak());

  return ClassFormat::fixedFields(fetchClass().as_object()->formatOfClass());
}


void Object::weakFinalizerCheckOf() {
  /* "Our oop has at least 2 non-weak fixed slots (this is assured before entering this method, in
   #finalizeReference:.
   We are assuming that if its first non-weak field is an instance of WeakFinalizer class,
   then we should add this oop to that list, by storing it to list's first field and
   also, updating the oop's 2nd fixed slot to point to the value which we overridden:
  */
  Oop list = fetchPointer(0);
  Oop listClass = list.fetchClass();
  if (listClass == The_Squeak_Interpreter()->splObj(Special_Indices::ClassWeakFinalizer)) {
    Object_p list_obj = list.as_object();
    Oop first = list_obj->fetchPointer(0);
    storePointer(1, first);
    list_obj->storePointer(0, as_oop());
  }
}


int Object::instance_variable_names_index_of_class(const char* some_instance_variable_name) {
  static int cached_index = -1;
  if (cached_index >= 0) return cached_index;
 
  for (Oop* oopp = as_oop_p();  oopp <= last_pointer_addr();  ++oopp) {

    int index = oopp - as_oop_p() - BaseHeaderSize/sizeof(Oop);
    Oop oop = *oopp;
    if (!oop.isArray())
      continue;
    Object_p array_obj = oop.as_object();
    int n = array_obj->fetchWordLength();
    bool found_var_name = false;
    bool found_non_string = false;
    for (int i = 0;  i < n;  ++i)  { 
      Oop contents = array_obj->fetchPointer(i);
      if (!contents.isBytes()  ||  contents.isContext())
        found_non_string = true;
      else if (contents.as_object()->equals_string(some_instance_variable_name))
        found_var_name = true;
    }
    if (found_var_name  &&  !found_non_string)
      return cached_index = index;
  }
  fatal("could not find instance variable name array in a class");
  return -1;
}



int Object::index_of_string_in_array(const char* aString) {
  for (int i = 0, n = fetchWordLength();  i < n;  ++i)
    if (fetchPointer(i).as_object()->equals_string(aString))
      return i;
  return -1;
}

