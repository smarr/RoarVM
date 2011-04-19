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


class Header_Type {
  // See object.h: both objects and chunks start with a word containing a header type

 public:

  static const int Shift = 0;
  static const int Mask = 3;
  static const int Width = 2; // 2 bits
  // type values
  static const int SizeAndClass = 0; // size and class separate in 3-word header
  static const int Class = 1; // class separate in 2-word header
	static const int Free  = 2; // free object, rest of header is size
	static const int Short = 3; // size and class index in 1 word-header

private:
  static char extra_bytes_without_preheader[4]; // extra header bytes by type, threadsafe, Stefan: is read only, 2009-09-06
  static char extra_bytes_with_preheader[4]; // extra header bytes by type, threadsafe, Stefan: is read only, 2009-09-06
  static char* extra_bytes() { return extra_bytes_with_preheader; }

  static char extra_oops_without_preheader[4]; // extra header oops by type, threadsafe, Stefan: is read only, 2009-09-06
  static char extra_oops_with_preheader[4]; // extra header oops by type, threadsafe, Stefan: is read only, 2009-09-06
  static char* extra_oops() { return extra_oops_with_preheader; }

public:

  static int extract_from(int32 word) { return word &  Mask; }
  static int without_type(int32 word) { return word & ~Mask; }

  static int extraHeaderBytes(int header_type) {
    //Return the number of extra bytes used by the given object's header."
    // Warning: This method should not be used during marking, when the header type bits of an object may be incorrect."
    // works on chunk or Oop
    return  extra_bytes()[header_type];
  }

  static int extraHeaderBytes_without_preheader(int header_type) {
    //Return the number of extra bytes used by the given object's header."
    // Warning: This method should not be used during marking, when the header type bits of an object may be incorrect."
    // works on chunk or Oop
    return  extra_bytes_without_preheader[header_type];
  }

  static int extraHeaderOops(int header_type) {
    return  extra_oops()[header_type];
  }

  static int extraHeaderOops_without_preheader(int header_type) {
    return  extra_oops_without_preheader[header_type];
  }

  static bool contains_sizeHeader(int32 word) { return extract_from(word) == SizeAndClass; }
  static bool contains_class_and_type_word(int32 word) { return extract_from(word)<= Class; }
  static bool is_free_object(int32 word) { return extract_from(word) == Free; }


};

