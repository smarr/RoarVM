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
 Pointer and Object Representation
 =================================

 This class describes a 32-bit direct-pointer object memory for Smalltalk.

 Pointer
 -------
 
 The model is very simple in principle:  
     a pointer is either a SmallInteger 
     or a 32-bit direct object pointer.

 SmallIntegers are tagged with a low-order bit equal to 1,
  and an immediate 31-bit 2s-complement signed value in the rest of the word.

 All object pointers point to a header, which may be followed by a number of 
 data fields.
 
 Thus, the pointer to an object always points at the begin of the _base header_,
 which is after additional variable-sized _extra headers_, and the RoarVM
 specific _preheaders_.

 
 Object Representation
 ---------------------
 
        === Object Representation in Memory, Including all Headers ===
        (Note: this ensemble is referred to as a Chunk (cfr. Chunk.h))
 +-------------+-----------------+--------------------+---------------------+
 |Pre (k words)|Extra (0-2 words)|Base Header (1 word)|Data/Fields (n words)|
 +-------------+-----------------+--------------------+---------------------+
                                 ↑
                                 Target of Object*|Object_p|Oop
 
 This object memory achieves considerable compactness by using
 a variable header size (the one complexity of the design).

 
 The format of the 0th (base-)header word is as follows:

	3 bits	reserved for gc (mark, root, unused)
	12 bits	object hash (for HashSets)
	5 bits	compact class index
	4 bits	object format
	6 bits	object size in 32-bit words
	2 bits	header type (0: 3-word, 1: 2-word, 2: forbidden, 3: 1-word)

 If a class is in the compact class table, then this is the only header
 information needed.  If it is not, then it will have another (extra-)header word at
 offset -4 bytes with its class in the high 30 bits, and the header type
 repeated in its low 2 bits.
 
 If the objects size is greater than 255 bytes, then it will have yet another
 (extra-)header word at offset -8 bytes with its full word size in the high 30 bits and
 its header type repeated in the low two bits.

 The object format field provides the remaining information as given in the
 formatOf: method (including isPointers, isVariable, isBytes,
 and the low 2 size bits of byte-sized objects).
 
 Note: the "Extra header" terminology used here is not to be confused with "extraHeaderBytes" 
       and "extraHeaderOops" in the code. These last terms refer to the accumulated size of 
       all of an object's headers before its Base header rather than the size of its "Extra header".

 
 Note on the original Squeak VM Garbage Collector 
  -----------------------------------------------
 
 Note: the following two lines were true of the original Squeak VM, 
 but are not true in this this VM as of 11/15/10. -- dmu & sm
 
 (This implementation includes incremental (2-generation) and full garbage
 collection, each with compaction and rectification of direct pointers.)
 
 It also supports a bulk-become (exchange object identity) feature that allows
 many objects to be becomed at once, as when all instances of a class must be
 grown or shrunk.

*/

class Object
# if !Work_Around_Extra_Words_In_Classes
: public Word_Containing_Object_Type
# endif

 {
   # if Work_Around_Extra_Words_In_Classes
  WORD_CONTAINING_OBJECT_TYPE_MEMBERS
  # endif

public:

  static oop_int_t primitiveIndex_of_header(oop_int_t header) {
    return ((header & Object_Indices::Primitive_Index_Low_Mask ) >> Object_Indices::Primitive_Index_Low_Shift)
        |  ((header & Object_Indices::Primitive_Index_High_Mask) >> Object_Indices::Primitive_Index_High_Shift);
  }
  static int MaxPrimitiveIndex() { return primitiveIndex_of_header(~0); }

  inline oop_int_t primitiveIndex();


  int32 baseHeader;
  static const int BaseHeaderSize = sizeof(int32);


  char*      as_char_p()     { return (char*)this; }
  u_char*    as_u_char_p()   { return (u_char*)this; }
  oop_int_t* as_oop_int_p()  { return (oop_int_t*)this; }
  int32*     as_int32_p()    { return (int32*)this; }
  Oop*       as_oop_p()      { return (Oop*)this; }

  bool contains_sizeHeader() {
    return Header_Type::contains_sizeHeader(baseHeader);
  }
  oop_int_t& sizeHeader() {
    assert(contains_sizeHeader());
    return as_oop_int_p()[-2];  // -2: See comment at the top, it is the extra header for which we need to adjust
  }
  bool contains_class_and_type_word() {
    return Header_Type::contains_class_and_type_word(baseHeader);
  }
  oop_int_t& class_and_type_word() { return as_oop_int_p()[-1]; } // -1: See comment at the top, it is the extra header for which we need to adjust
   
  Oop  get_class_oop() {
    Oop r = Oop::from_bits(Header_Type::without_type(class_and_type_word()));
    if (check_many_assertions)
      assert(r == Oop::from_mem_bits(u_oop_int_t(class_and_type_word()) >> Header_Type::Width));
    return r;
  }
  void set_class_oop(Oop x);
  void set_class_oop_no_barrier(Oop);

# if Has_Preheader
  Preheader* preheader() { 
    return  (Preheader*)&as_oop_int_p()[-extra_header_oops()];
  }
  
  oop_int_t* extra_preheader_word() {
    return preheader()->extra_preheader_word_address();
  }
  
  oop_int_t get_extra_preheader_word() { return *extra_preheader_word(); }
  
  void init_extra_preheader_word() { preheader()->init_extra_preheader_word(); }

  void set_preheader(Oop x) { 
    init_extra_preheader_word();
    # if Enforce_Backpointer || Use_Object_Table
      set_backpointer(x);
    # endif
  }

# else // !Has_Preheader
  inline void set_preheader(Oop) const {}
  inline void init_extra_preheader_word() const {}
  oop_int_t* extra_preheader_word() const { return NULL; }

# endif // Has_Preheader
   
  inline void set_extra_preheader_word(oop_int_t w);
   
   
# if Enforce_Backpointer || Use_Object_Table   
   Oop backpointer() { return oop_from_backpointer(get_backpointer_word()); }
   
   void set_backpointer(Oop x) {
     set_backpointer_word(backpointer_from_oop(x));
   }
   
   static Oop oop_from_backpointer(oop_int_t bp) {
     return Oop::from_mem_bits(u_oop_int_t(bp) >> Header_Type::Width);
   }
   
   oop_int_t backpointer_from_oop(Oop x) {
     return (x.mem_bits() << Header_Type::Width) | (headerType() << Header_Type::Shift);
   }
   
   oop_int_t get_backpointer_word() { return *backpointer_word(); }
   
   inline void set_backpointer_word(oop_int_t w);
   
   oop_int_t* backpointer_word() {
     return &preheader()->backpointer;
   }
# else
  inline void set_backpointer(Oop) const {}
   
# endif

public:

  static const int SizeShift = Header_Type::Width + Header_Type::Shift; // but never used since size is in words, and we need it by bytes anyway
  static const int SizeWidth = 6;
  static const int SizeMask = ((1 << SizeWidth) - 1) << SizeShift; // should be 0xfc
  static const int Size4Bit = 0; // changes for 64 bit oops

  static const int LongSizeMask = ~Header_Type::Mask;

  inline oop_int_t sizeBits();
  inline oop_int_t sizeBitsSafe();
  oop_int_t shortSizeBits() { return baseHeader & SizeMask; }
  oop_int_t longSizeBits()  { return sizeHeader() & LongSizeMask; }
  oop_int_t total_byte_size();
  oop_int_t total_byte_size_without_preheader();

  oop_int_t stSize();

  static const int FormatShift = SizeShift + SizeWidth;
  static const int FormatWidth = 4;
  static const int FormatMask = ((1 << FormatWidth) - 1) << FormatShift;


  static const int CompactClassShift = FormatShift + FormatWidth;
  static const int CompactClassWidth = 5;
  static const int CompactClassMask = ((1 << CompactClassWidth) - 1) << CompactClassShift; // should be 0x1f000

  oop_int_t compact_class_index() { return (baseHeader & CompactClassMask) >> CompactClassShift; }


  static const int HashShift = CompactClassShift + CompactClassWidth; // should be 17
  static const int HashWidth = 12;
  static const int HashBitsOffset = HashShift; // squeak name
  static const int HashMask = ((1 << HashWidth) - 1) << HashShift;
  static const int HashBits = HashMask; // squeak name, should be 0x1ffe0000;


  // "masks for root and mark bits"
  static const int UnusedWidth = 1;
	static const int RootShift = HashShift + HashWidth + UnusedWidth; // "Next-to-Top bit"
	static const int MarkShift = RootShift + 1;  // "Top bit"
	static const int32 RootBit = 1 << RootShift;
	static const int32 MarkBit = 1 << MarkShift;

  static bool verify_constants() {
    assert_eq(SizeMask,                0xfc, "");
    assert_eq(FormatShift, 8, "");
    assert_eq(FormatMask,             0xf00, "");
    assert_eq(CompactClassMask,     0x1f000, "");
    assert_eq(HashBits,          0x1ffe0000, "");
    assert_eq(RootShift, 30, "");
    assert_eq(MarkShift, 31, "");
    return true;
  }


	static const int LargeContextBit = 0x40000; // "This bit set in method headers if large context is needed."
  static Oop NilContext() { return Oop::from_int(0); }  // "the oop for the integer 0; used to mark the end of context lists"

  bool isHandlerMarked() { // for contexts
    if (!isMethodContext())  return false;
    Oop m =  fetchPointer(Object_Indices::MethodIndex);
    return m.is_mem()  &&  m.as_object()->primitiveIndex() == 199;
  }

 public:
  bool is_marked() { return header_is_marked(baseHeader); }
  static bool header_is_marked(int32 hdr) { return hdr & MarkBit; }

  inline void   mark_without_store_barrier();
  inline void unmark_without_store_barrier();

 public:

  Oop as_oop() { return Oop::from_object(this); }

  inline static int rightType(oop_int_t headerWord);


   static oop_int_t make_free_object_header(oop_int_t bytes_including_header) {
     return round_up_by_power_of_two(bytes_including_header, sizeof(Oop)) | Header_Type::Free;
   }

  oop_int_t sizeOfFree() { return Header_Type::without_type(baseHeader); }

  bool isFreeObject() { return is_free(); } // Squeakish name

  Chunk* my_chunk()  { return my_chunk(extra_header_bytes()); }
  Chunk* my_chunk(int extra_header_bytes) { return (Chunk*)&as_char_p()[-extra_header_bytes]; }
  Chunk* nextChunk() {
    return (Chunk*)&as_char_p()[bytes_to_next_chunk()];
  }

  
  Object* nextObject();
  Oop nextInstance();  
  
  Object* nextAccessibleObject() {
    for(Object* curr = nextObject();curr != NULL;curr = curr->nextObject()) {
      if (!curr->isFreeObject()) {
        return curr;
      }
    }
    return NULL;
  }
  

  

  Chunk* my_chunk_without_preheader()  { return (Chunk*)&as_char_p()[-extra_header_bytes_without_preheader()]; }

  oop_int_t bytes_to_next_chunk() { return isFreeObject() ? sizeOfFree() : sizeBits(); }

  void* firstFixedField() { return as_char_p() + BaseHeaderSize; }


  class Format {
    /*
     "       0      no fields
     1      fixed fields only (all containing pointers)
     2      indexable fields only (all containing pointers)
     3      both fixed and indexable fields (all containing pointers)
     4      both fixed and indexable weak fields (all containing pointers).

     5      unused
     6      indexable word fields only (no pointers)
     7      indexable long (64-bit) fields (only in 64-bit images)

     8-11      indexable byte fields only (no pointers) (low 2 bits are low 2 bits of size)
     12-15     compiled methods:
     # of literal oops specified in method header,
     followed by indexable bytes (same interpretation of low 2 bits as above)
     "
     */
  public:
    static const int no_fields = 0;
    static const int fixed_fields_only = 1; // all pointers
    static const int indexable_fields_only = 2; // all pointers
    static const int both_fixed_and_indexable_fields = 3; // all pointers
    static const int both_fixed_and_indexable_weak_fields = 4; // all pointers
    static const int unused = 5;
    static const int indexable_word_fields_only = 6; // no pointers
    static const int indexable_long_fields_only = 7; // only in 64-bit images
    static const int indexable_byte_fields_only_0 = 8; // low 2 bits are low 2 bits of size
    static const int indexable_byte_fields_only_1 = 8 + 1; // low 2 bits are low 2 bits of size
    static const int indexable_byte_fields_only_2 = 8 + 2; // low 2 bits are low 2 bits of size
    static const int indexable_byte_fields_only_3 = 8 + 3; // low 2 bits are low 2 bits of size
    static const int compiled_method_0 = 12 + 0; // # literal oops speced in method header followed by indexable bytes
    static const int compiled_method_1 = 12 + 1; // # literal oops speced in method header followed by indexable bytes
    static const int compiled_method_2 = 12 + 2; // # literal oops speced in method header followed by indexable bytes
    static const int compiled_method_3 = 12 + 3; // # literal oops speced in method header followed by indexable bytes
    static const int byte_size_bits = 3;

    static const int first_byte = indexable_byte_fields_only_0;

   public:
    static bool has_fields(int fmt) { return fmt != 0; }
    static bool has_only_fixed_fields(int fmt) { return fmt <= fixed_fields_only; }
    static bool has_bytes(int fmt)  { return fmt >= first_byte; }
    static bool isCompiledMethod(int fmt) { return fmt >= compiled_method_0; }
    static bool isWeak(int fmt) { return fmt == both_fixed_and_indexable_weak_fields; }
    static bool has_only_oops(int fmt) { return fmt <= both_fixed_and_indexable_weak_fields; }
    static bool might_be_context(int fmt) { return fmt == both_fixed_and_indexable_fields; }
    static bool has_no_oops(int fmt) { return fmt >= indexable_word_fields_only  &&  fmt <= indexable_byte_fields_only_3; }
    static bool has_only_indexable_fields(int fmt) {
      return  fmt >= indexable_word_fields_only
         ||   fmt == indexable_fields_only;
    }
    static bool isArray(int fmt) { return fmt == indexable_fields_only; }
    static bool is_valid(int fmt) { return fmt != unused  &&  fmt != indexable_long_fields_only; }
    static bool isWordsOrBytes(int fmt) {
      return fmt == indexable_word_fields_only
      ||   ( has_bytes(fmt)  &&  !isCompiledMethod(fmt) );
    }
    static bool isIndexable(int fmt) { return fmt >= indexable_fields_only; }
    static bool isWords(int fmt) { return fmt == indexable_word_fields_only; }
  };


  class CompactClass {
   public:
    static const oop_int_t MethodContext = 14; // ST VM switches these!
    static const oop_int_t  BlockContext = 13;
    static const oop_int_t PseudoContext =  4;

    static bool isContextHeader(oop_int_t aHeader) {
      oop_int_t h = aHeader & CompactClassMask;
      return  h == (MethodContext << CompactClassShift)
          ||  h == ( BlockContext << CompactClassShift)
          ||  h == (PseudoContext << CompactClassShift);
    }
    static bool isMethodContextHeader(oop_int_t aHeader) {
      return (aHeader & CompactClassMask) == (MethodContext << CompactClassShift);
    }
  };



  bool is_this_context_a_block_context() { return fetchPointer(Object_Indices::MethodIndex).is_int(); }

  bool isMethodContext() { return CompactClass::isMethodContextHeader(baseHeader); }
  bool hasContextHeader() { return CompactClass::isContextHeader(baseHeader); }
  bool hasSender(Oop);
  Object_p home_of_block_or_method_context() {
    return is_this_context_a_block_context() ? fetchPointer(Object_Indices::HomeIndex).as_object() : (Object_p)this;
  }
  Oop key_at_identity_value(Oop);

  void byteSwapIfByteObject();
  inline char* first_byte_address();
  inline int32 methodHeader(); // use instead of header()
  inline void beRootIfOld();
  bool is_new() { /*unimplemented();*/ return false; }


  // ObjectMemory object enumeration
  Oop* last_pointer_addr();
  Oop* last_strong_pointer_addr();
  Oop* last_strong_pointer_addr_remembering_weak_roots(Abstract_Mark_Sweep_Collector*);
  oop_int_t lastPointer();
  oop_int_t nonWeakFieldsOf();

  // ObjectMemory initialization
  void do_all_oops_of_object(Oop_Closure*, bool do_checks = check_assertions);
  void do_all_oops_of_object_for_reading_snapshot(Squeak_Image_Reader* r);
  void do_all_oops_of_object_for_marking(Abstract_Mark_Sweep_Collector*, bool do_checks = check_assertions);

  Oop clone();

  // ObjectMemory interpreter access
  inline Object_p instantiateContext(oop_int_t byteSize);

  // ObjectMemory header access
  int32 classHeader() { return class_and_type_word(); }
  oop_int_t format() {
    assert(FormatMask > 0);
    return (baseHeader & FormatMask) >> FormatShift;
  }
  int32 hashBits() { return (baseHeader & HashMask) >> HashShift; }

  inline bool isArray();
  inline bool isBytes();
  inline bool isPointers();
  bool isFloatObject();
  inline bool isWordsOrBytes();
  bool isIndexable() { return Format::isIndexable(format()); }
  bool isWeak() { return Format::isWeak(format()); }
  bool isWords() { return Format::isWords(format()); }



  inline bool isUnwindMarked();




  private:
  inline Oop& pointer_at(oop_int_t fieldIndex);
  u_char& byte_at(oop_int_t byteIndex);
  int32& long32_at(oop_int_t fieldIndex);

  public:
  inline Oop  fetchPointer(oop_int_t fieldIndex);
  inline void storePointer(oop_int_t fieldIndex, Oop oop);
  inline void storePointerUnchecked( oop_int_t fieldIndex, Oop oop);
  inline void storePointerIntoContext(oop_int_t i, Oop x);

  void storePointerIntoYoung(oop_int_t i, Oop x) {
    storePointerUnchecked(i, x);
  }

  inline u_char fetchByte(oop_int_t byteIndex);
  inline void storeByte( oop_int_t byteIndex, u_char valueByte);

  inline oop_int_t fetchLong32(oop_int_t fieldIndex);
  inline void storeLong32(oop_int_t fieldIndex, int32 x);

  oop_int_t fetchInteger(oop_int_t fieldIndex);
  void storeInteger(oop_int_t fieldIndex, oop_int_t x);
  void storeIntegerUnchecked(oop_int_t fieldIndex, oop_int_t x) {
    storePointerUnchecked(fieldIndex, Oop::from_int(x));
  }
  void storeIntegerUnchecked_into_context(oop_int_t fieldIndex, oop_int_t x) {
    storePointerUnchecked(fieldIndex, Oop::from_int(x));
  }


  inline double fetchFloatAtinto();
  double fetchFloatofObject(oop_int_t fieldIndex);


  oop_int_t fetchLong32Length();
  oop_int_t fetchWordLength() {
    // size appropriate for fetchPointer, but not in general for fetchLong32, etc
    return (sizeBits() - BaseHeaderSize) >> ShiftForWord;
  }

  void* fetchArray(oop_int_t fieldIndex) {
    return fetchPointer(fieldIndex).arrayValue();
  }


  static Oop positive32BitIntegerFor(u_int32 integerValue);
  static Oop   signed32BitIntegerFor(int32 integerValue);
  static Oop positive64BitIntegerFor(u_int64 integerValue);
  static Oop   signed64BitIntegerFor(int64 integerValue);


  inline oop_int_t quickFetchInteger(oop_int_t fieldIndex);



  inline Oop fetchClass();
  Oop className();
  Oop name_of_class_or_metaclass(bool* is_meta);

  inline u_oop_int_t lengthOf();
  oop_int_t byteSize() { return isBytes() ? slotSize() : oop_int_t(slotSize() * sizeof(Oop)); }
  inline oop_int_t byteLength();
  oop_int_t slotSize() { return lengthOf(); }
  inline u_oop_int_t fixedFieldsOfArray();
  inline void* arrayValue();
  inline oop_int_t formatOfClass();

  class ClassFormat {
   public:
    // from fixedFieldsOfArray
    static u_oop_int_t fixedFields(oop_int_t cfmt) { return ((cfmt >> 11) & 0xc0) + ((cfmt >> 2) & 0x3f) - 1; }
  };


  inline oop_int_t fetchStackPointer(); // rcvr is a ContextObject


  Object_p instantiateSmallClass(oop_int_t sizeInBytes);
  Object_p instantiateClass(oop_int_t sizeInBytes, Logical_Core* where = NULL);
  oop_int_t instanceSizeOfClass();

# if Enforce_Backpointer || Use_Object_Table
  inline void set_object_address_and_backpointer(Oop x  COMMA_DCL_ESB);
# endif

  inline bool isCompiledMethod();

  void flushExternalPrimitive();


  inline oop_int_t literalCount();
  inline Oop literal(oop_int_t offset);
  inline static oop_int_t literalCountOfHeader(oop_int_t header);
  oop_int_t argumentCount() { // of method
    return argumentCountOfHeader(methodHeader());
  }
  inline static oop_int_t argumentCountOfHeader(oop_int_t header);
  inline static oop_int_t temporaryCountOfHeader(oop_int_t header);
  oop_int_t temporaryCount() { return temporaryCountOfHeader(methodHeader()); }


  void cleanup_session_ID_and_ext_prim_index_of_external_primitive_literal();
  Object_p get_external_primitive_literal_of_method();

  inline Oop superclass();
  Oop methodClass() {
    return literal(literalCount() - 1).as_object()->fetchPointer(Object_Indices::ValueIndex);
  }



  inline void synchronousSignal(const char*);
  inline int priority_of_process();
  inline Oop name_of_process();
  inline Oop my_list_of_process();
  int core_where_process_is_running();
  int priority_of_process_or_nil();
  Object_p process_list_for_priority_of_process();
  Oop get_suspended_context_of_process_and_mark_running();
  bool is_process_running();
  bool is_process_allowed_to_run_on_this_core();
  void store_host_core_of_process(int);
  void store_allowable_cores_of_process(u_int64 bitMask);
  void add_process_to_scheduler_list();
  void set_suspended_context_of_process(Oop ctx);
  Oop removeFirstLinkOfList();
  Oop removeLastLinkOfList(Object_p);
  Oop removeMiddleLinkOfList(Object_p, Object_p);
  Oop remove_process_from_scheduler_list(const char*);
  void kvetch_nil_list_of_process(const char*);
  void addLastLinkToList(Oop);
  void nil_out_my_list_and_next_link_fields_of_process();



  bool isEmptyList();
  inline static Oop floatObject(double);
  inline void storeFloat(double);

   /** Floats are stored in platform order in Cog images.
    This function here is used during image load to make sure that
    the floats are stored in normalized, i.e., swaped order, since
    the standard interpreter and the RoarVM do not use platform order.
    
    REM: should NOT be called in normal operation. */
  inline void swapFloatParts_for_cog_compatibility();
   
  void storeStackPointerValue(oop_int_t v) {
    storePointerUnchecked(Object_Indices::StackPointerIndex, Oop::from_int(v));
  }


  private:
  inline char* first_byte_address_after_header();
  public:
  void print(Printer* p = dittoing_stdout_printer);
  void print_class(Printer*);
  void print_bytes(Printer*);
  void print_bytes_in_array(Printer*);
  void print_compiled_method(Printer*);
  void print_frame(Printer*);
  void print_process_or_nil(Printer*, bool print_stack = false);
  bool verify_process();
  bool selector_and_class_of_method_in_me_or_ancestors(Oop, Oop*, Oop*);

  inline bool equals_string(const char*);
  inline bool starts_with_string(const char*);
  void dp();


  bool verify();
  bool verify_address();
  bool verify_preheader();
  bool verify_extra_preheader_word();
  
# if Enforce_Backpointer || Use_Object_Table
  bool verify_backpointer();
# endif
  
  bool okayOop();
  bool hasOkayClass();


  static Object_p makePoint(oop_int_t, oop_int_t);
  static Object_p makeString(const char* str);
  static Object_p makeString(const char* str, int n);

  void* firstIndexableField_for_primitives();
  char* pointerForOop_for_primitives();

  oop_int_t argumentCountOfBlock();
# if Include_Closure_Support
   oop_int_t argumentCountOfClosure();
# endif
  static oop_int_t sizeOfSTArrayFromCPrimitive(void* p);

  bool is_current_copy() { return as_oop().as_object() == this; }

  inline bool is_suitable_for_replication();
  inline int  mutability_for_snapshot_object();

  void check_IP_in_context();
  void check_all_IPs_in_chain();
  u_char* next_bc_to_execute_of_context();
  void check_IP_of_method(u_char* bcp, Object_p);

  inline void save_block_method_and_IP();
  inline Oop  get_orig_block_method();
  inline Object*  get_orig_block_home();
  inline Oop  get_original_block_IP();
  inline void zapping_ctx();

  inline void set_count_of_blocks_homed_to_this_method(oop_int_t x);
  inline oop_int_t get_count_of_blocks_homed_to_this_method_ctx();

  inline void catch_stores_of_method_in_home_ctxs(Oop*,int,Oop);
   
  static inline bool image_is_pre_4_1();

  void weakFinalizerCheckOf();
   
  int instance_variable_names_index_of_class(const char*);
  static Object_p instance_variable_names_of_Process();
  int index_of_string_in_array(const char* aString);
  

  static void test();
};

# define FOR_EACH_OOP_IN_OBJECT_EXCEPT_CLASS(obj, oop_ptr) \
  for (Oop* oop_ptr = (obj)->last_pointer_addr();  oop_ptr > (obj)->as_oop_p();  --oop_ptr)


# define FOR_EACH_STRONG_OOP_IN_OBJECT_EXCEPT_CLASS_RECORDING_WEAK_ROOTS(obj, oop_ptr, gc) \
  for (Oop* oop_ptr = (obj)->last_strong_pointer_addr_remembering_weak_roots(gc);  oop_ptr > (obj)->as_oop_p();  --oop_ptr)

# define FOR_EACH_WEAK_OOP_IN_OBJECT(obj, oop_ptr) \
  for ( Oop *oop_ptr  = obj->last_strong_pointer_addr() + 1, \
      *last_oop_ptr  =  obj->last_pointer_addr(); \
            oop_ptr <= last_oop_ptr ; \
          ++oop_ptr)

