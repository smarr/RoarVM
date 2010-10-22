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


class Int_Oop;
class Mem_Oop;

typedef int32 oop_int_t;

# define oop_int_t_contents int32_contents

class Oop

# if !Work_Around_Extra_Words_In_Classes

: public Abstract_Oop

# endif

{
private:
  oop_int_t _bits;
  Oop(oop_int_t b) { this->_bits = b; }

 public:
  class Illegals {
    public:
    // should all be even to look like bad pointers
    static const oop_int_t uninitialized = 0xbadbad00;
    static const oop_int_t magic = 0xabcdabdc;
    static const oop_int_t shifted = 0xbadcad00; // low-order bits are different
    static const oop_int_t zapped = 0xdeaddeac;
    static const oop_int_t trimmed_end = 0xdead1100;
    static const oop_int_t allocated = 0xfefefefe;
    static const oop_int_t made_free = 0xd0dedede; // 2nd nibble is different; look for dede
    static const oop_int_t free_extra_preheader_words = 0xe0e0e0e0;
  };



  static Oop from_bits(oop_int_t b) { return Oop(b); }
  static Oop from_int(oop_int_t i ) { return Oop((i << Tag_Size) | Int_Tag); }
  inline static Oop from_object(Object* p);
  inline static Oop from_mem_bits(u_oop_int_t mem_bits);



  Oop() { _bits = Illegals::uninitialized & ~Tag_Mask  |  Mem_Tag; } // illegal


  inline oop_int_t bits() { return _bits; }
  oop_int_t bits_for_hash() { return u_oop_int_t(bits()) >> ShiftForWord;   } // for method cache
  oop_int_t integerValue() { assert(is_int());  return _bits >> Tag_Size; }
  static bool isIntegerValue(oop_int_t i) { return ((i << Tag_Size) >> Tag_Size) == i; }
  inline oop_int_t mem_bits();


  inline oop_int_t checkedIntegerValue();

  bool operator == (Oop x) {  return bits() == x.bits(); }
  bool operator != (Oop x) {  return bits() != x.bits(); }

  bool is_mem() { return (bits() & Tag_Mask) == Mem_Tag; }
  bool is_int() { return (bits() & Tag_Mask) == Int_Tag; }

  inline Object* as_object();
  inline Object* as_object_unchecked();
  inline Object* as_object_if_mem();

  inline Oop fetchClass();
  bool isMemberOf(char*);
  bool isKindOf(char*);

  // subclass forwarders:

  inline bool verify_oop();
  inline bool verify_object();
  bool verify_object_or_null() { return bits() == 0  ||  verify_object(); }
  bool okayOop();


  void print(Printer* p = dittoing_stdout_printer);
  void print_process_or_nil(Printer* p);
  void print_briefly(Printer*); // used for slot contents
  void dp();

  bool is_new() { return false; /* unimplemented */ }

  // ObjectMemory headerAccess
  bool isPointers();
  bool isBytes();
  bool isWordsOrBytes();
  bool isArray();
  bool isIndexable();
  bool isWeak();
  bool isWords();
  bool isContext();

  inline void* arrayValue();

  oop_int_t byteSize();
  oop_int_t slotSize();

  // Object Memory allocation
  Oop beRootIfOld() { /* unimplemented */ return *this; }

  inline int  rank_of_object();
  inline int  mutability();

  static void test();
};

static const int bytes_per_oop = sizeof(Oop);


inline int convert_byte_count_to_oop_count(int x) {
  return divide_by_power_of_two_and_round_up(x, bytes_per_oop);
}



inline void oopcpy_no_store_check(Oop* dst, const Oop* src, int n, Object* dstObj);
inline void oopset_no_store_check(Oop* dst, Oop src, int n);

