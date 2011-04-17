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


typedef int32 mc_word;

/*
 "This class implements a simple method lookup cache. If an entry for the
  given selector and class is found in the cache, set the values of
  'newMethod' and 'primitiveIndex' and return true. Otherwise, return false."
  
 "About the re-probe scheme: The hash is the low bits of the XOR of two large
  addresses, minus their useless lowest two bits. If a probe doesn't get a
  hit, the hash is shifted right one bit to compute the next probe,
  introducing a new randomish bit. The cache is probed CacheProbeMax times
  before giving up."
  
 "WARNING: Since the hash computation is based on the object addresses of the
  class and selector, we must rehash or flush when compacting storage. We've
  chosen to flush, since that also saves the trouble of updating the addresses
  of the objects in the cache."
 */

class Method_Cache {
 public:
  class entry {
   public:
    Oop selector;
    Oop klass;
    Oop method;
    int prim; // index
    Oop native;
    fn_t primFunction;
    bool do_primitive_on_main;

    bool matches(Oop s, Oop k) {return s == selector  &&  k == klass  ?  this  : NULL; }

    bool is_empty() { return selector.bits() == 0; }
    void be_empty() { selector = Oop::from_bits(0); }
    void set_from(Oop sel, Oop k, Oop m, int p, Oop n, fn_t pf, bool om) {
      selector = sel;  klass = k;  method = m;  native = n; prim = p;  primFunction = pf; do_primitive_on_main = om;
    }
    bool verify() {
      return is_empty()
      ||    selector.verify_object()  &&  klass.verify_object()
          &&  method.verify_object()  &&  native.verify_object_or_null();
    }
  };
 private:
  static const int EntryWordsRoundedUp = 8;
  static const int EntryWRUShift = 3;
  static const oop_int_t Entries = 512;
  static const int CacheProbeMax = 3;

  mc_word words[EntryWordsRoundedUp * Entries];

  entry* at(int hash) {
    return (entry*)&words[(hash & (Entries - 1)) * EntryWordsRoundedUp];
  }

  int hash_of(Oop sel, Oop klass) { return sel.bits_for_hash() ^ klass.bits_for_hash(); }

 # define FOR_EACH_PROBE(sel, klass, i, hash, e) \
  int hash = hash_of(sel, klass); \
  entry* e; \
  for (int i = 0;  e = at(hash), i < CacheProbeMax;  ++i, hash >>= 1)

 public:
  void flush_method_cache() {
    assert(sizeof(entry)  <=  EntryWordsRoundedUp * sizeof(mc_word));
    memset(words, 0, sizeof(words));
  }


  entry* at(Oop selector, Oop klass) {
    FOR_EACH_PROBE(selector, klass, i, hash, e)
      if (e->matches(selector, klass))
        return e;
    return NULL;
  }

  void addNewMethod(Oop sel, Oop klass, Oop method, int prim, Oop native, fn_t primFunction, bool on_main);
  void rewrite(Oop sel, Oop klass, int prim);
  void rewrite(Oop sel, Oop klass, int prim, fn_t primFunction, bool on_main);

  void flushByMethod(Oop method) {
    for (int i = 0;  i < Entries;  ++i) {
      entry* p = at(i);
      if (p->method == method)
        p->selector = Oop::from_int(0);
    }
  }


  void flushSelective(Oop sel) {
    for (int i = 0;  i < Entries;  ++i) {
      entry* p = at(i);
      if (p->selector == sel)
        p->selector = Oop::from_int(0);
    }
  }

  bool verify();

};

