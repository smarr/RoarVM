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


inline void oopcpy_no_store_check(Oop* const dst, const Oop* src, int n, Object_p dst_obj_to_be_evacuated) {
  assert(The_Memory_System()->contains(dst));

  The_Memory_System()->enforce_coherence_before_store_into_object_by_interpreter(dst, n << ShiftForWord, dst_obj_to_be_evacuated);
  DEBUG_MULTIMOVE_CHECK(dst, src, n);
  memmove(dst, src, n * bytes_per_oop);
  The_Memory_System()->enforce_coherence_after_store_into_object_by_interpreter(dst, n << ShiftForWord);
}

inline void oopset_no_store_check(Oop* const dst, Oop src, int n) {
  assert(The_Memory_System()->contains(dst));
  The_Memory_System()->enforce_coherence_before_store(dst, n << ShiftForWord); // not into an object we need
  DEBUG_MULTISTORE_CHECK(dst, src, n);
  
  Oop* destination = dst;
  for (int i = 0;  i < n;  ++i)  {
    *destination++ = src;
  }
  The_Memory_System()->enforce_coherence_after_store(dst, n << ShiftForWord);
}

inline Oop Oop::fetchClass() {
  return is_int() ? The_Squeak_Interpreter()->splObj(Special_Indices::ClassInteger)
  : as_object()->fetchClass();
}


inline bool Oop::isPointers()     { return is_mem()  &&  as_object()->isPointers(); }
inline bool Oop::isBytes()        { return is_mem()  &&  as_object()->isBytes(); }
inline bool Oop::isWordsOrBytes() { return is_mem()  &&  as_object()->isWordsOrBytes(); }
inline bool Oop::isArray()        { return is_mem()  &&  as_object()->isArray(); }
inline bool Oop::isIndexable()    { return is_mem()  &&  as_object()->isIndexable(); }
inline bool Oop::isWeak()         { return is_mem()  &&  as_object()->isWeak(); }
inline bool Oop::isWords()        { return is_mem()  &&  as_object()->isWords(); }
inline bool Oop::isContext()      { return is_mem()  &&  as_object()->hasContextHeader(); }



inline Oop Oop::from_object(Object* p) {
  if (check_many_assertions)
    assert_message(p != NULL,
                   "used to count on being able to do this, fix these uses");
# if Use_Object_Table
  return p->backpointer();
# else
  assert_eq((oop_int_t)p, (oop_int_t)p | Mem_Tag, "They should already be tagged.");
  # warning STEFAN: check whether we actually need the OR here
  return from_bits((oop_int_t)p | Mem_Tag);
# endif
}


// TODO: perhaps it should be moved to the point right after getting it out
//       of the object table...
inline Object_p Oop::as_object_unchecked() {
# if Use_Object_Table
  return (Object_p)The_Memory_System()->object_for_unchecked(*this);
# else
  return as_object();
# endif
}

inline Object* Oop::as_untracked_object_ptr() {
# if Use_Object_Table
  return The_Memory_System()->object_for(*this);
# else
  return as_object();
# endif
}

inline Object_p Oop::as_object() {
# if Use_Object_Table
  return (Object_p)The_Memory_System()->object_for(*this);
# else
  return (Object_p)(Object*)bits();
# endif
}

inline Object_p Oop::as_object_if_mem() {
  return is_mem() ? as_object() : (Object_p)NULL;
}



inline Oop Oop::from_mem_bits(u_oop_int_t mem_bits) { return Oop((mem_bits << Header_Type::Width) | Mem_Tag); }
inline oop_int_t Oop::mem_bits() { return u_oop_int_t(bits()) >> Header_Type::Width; }

inline oop_int_t Oop::checkedIntegerValue() {
  return is_int() ? integerValue() : (The_Squeak_Interpreter()->primitiveFail(), 0);
}




inline bool Oop::verify_oop() {
  return !is_mem()  ||  as_object()->verify_address();
}
inline bool Oop::verify_object() {
  return !is_mem()  ||  as_object()->verify();
}

inline bool Oop::okayOop() { return  !is_mem()  ||  as_object()->okayOop(); }



inline oop_int_t Oop::byteSize() { return is_int() ? 0 : as_object()->byteSize(); }
inline oop_int_t Oop::slotSize() { return is_int() ? 0 : as_object()->lengthOf(); }

inline void* Oop::arrayValue() {
  return is_mem() ? as_object()->arrayValue() : (void*)(The_Squeak_Interpreter()->primitiveFail(), 0);
}

inline int  Oop::rank_of_object()       {  return is_int()  ?  Logical_Core::my_rank()          :  The_Memory_System()->     rank_for_address(as_object()); }
inline int  Oop::mutability()           {  return is_int()  ?  Memory_System::read_mostly  :  The_Memory_System()->mutability_for_address(as_object()); }


