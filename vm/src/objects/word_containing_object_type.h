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


// see comment at top of object.h

/** This header uses the fact that every header should normally be included
    only once, in the beginnging by headers.h in the RoarVM codebase.
 
    After it is included the first time, an include guard will be defined,
    that allows to reinclude the file later again without redefining the class,
    but only inlining the method definitions instead.
 
    This is done to avoid compiler problems on some platforms, where additional
    words are introduced into C++ objects when inheritance is used.
 */

# ifndef __WORD_CONTAINING_OBJECT_TYPE_MEMBERS__PARTICAL_INCLUDE_GUARD__

class Word_Containing_Object_Type {
  
# endif
  
private:
  /* could be object header word, size header word,
    class header word, backpointer, or any other preheader word  */
  oop_int_t some_word_in_an_object_header() { return *(oop_int_t*)this; }

public:

  int headerType() {
    return Header_Type::extract_from(some_word_in_an_object_header());
  }
 
  int extra_header_bytes() {
    return Header_Type::extraHeaderBytes(headerType());
  }
 
  int extra_header_bytes_without_preheader() {
    return Header_Type::extraHeaderBytes_without_preheader(headerType());
  }
 
  int extra_header_oops() {
    return Header_Type::extraHeaderOops(headerType());
  }
 
  int extra_header_oops_without_preheader() {
    return Header_Type::extraHeaderOops_without_preheader(headerType());
  }
 
  bool is_free() {
    return Header_Type::Free == headerType();
  }

# ifndef __WORD_CONTAINING_OBJECT_TYPE_MEMBERS__PARTICAL_INCLUDE_GUARD__

};

// Here we eventually define the include guard, which should be after
// the first usage of this header.
# define __WORD_CONTAINING_OBJECT_TYPE_MEMBERS__PARTICAL_INCLUDE_GUARD__

# endif



