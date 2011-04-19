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


class At_Cache {
  static const int Num_Entries = 1 << 3; // must be power of two
 public:
  class Entry {
   public:
    Oop oop;
    oop_int_t size;
    oop_int_t fmt;
    oop_int_t fixedFields;

    void flush() { oop = Oop::from_bits(0); }
    void install(Oop x, bool stringy);
    bool matches(Oop x) { return oop == x; }
    bool verify() { return oop.verify_object_or_null(); }
  } ats[Num_Entries], at_puts[Num_Entries];
  Entry* get_entry(Oop rcvr, bool isPut) {
    return &(isPut ? at_puts : ats)[rcvr.bits_for_hash() & (Num_Entries - 1)];
  }
  void flush_at_cache() {
    for (int i = 0;  i < Num_Entries;  ++i){
      ats[i].flush();
      at_puts[i].flush();
    }
  }

  bool verify() {
    for (int i = 0;  i < Num_Entries; ++i) {
      ats[i].verify();
      at_puts[i].verify();
    }
    return true;
  }
};

