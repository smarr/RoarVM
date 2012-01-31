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

void Method_Cache::addNewMethod(Oop sel, Oop klass, Oop method, int prim, Oop native, fn_t primFunction, bool on_main) {
  /*
   "Add the given entry to the method cache.
   The policy is as follows:
   Look for an empty entry anywhere in the reprobe chain.
   If found, install the new entry there.
   If not found, then install the new entry at the first probe position
   and delete the entries in the rest of the reprobe chain.
   This has two useful purposes:
   If there is active contention over the first slot, the second
   or third will likely be free for reentry after ejection.
   Also, flushing is good when reprobe chains are getting full."
   */
  FOR_EACH_PROBE(sel, klass, i, hash, e) {
    if (e->is_empty()) {
      // Found an empty entry -- use it
      e->set_from(sel, klass, method, prim, native, primFunction, on_main);
      return;
    }
  }
  // "OK, we failed to find an entry -- install at the first slot..."
  // "...and zap the following entries"
  FOR_EACH_PROBE(sel, klass, i, hash2, e2) {
    if (i == 0)
      e2->set_from(sel, klass, method, prim, native, primFunction, on_main);
    else
      e2->be_empty();
  }
}

void Method_Cache::rewrite(Oop sel, Oop klass, int localPrimIndex) {
  rewrite(sel, klass, localPrimIndex,
          localPrimIndex == 0 ? NULL  : primitiveTable.contents[localPrimIndex],
          localPrimIndex == 0 ? false : primitiveTable.execute_on_main[localPrimIndex]);
}

void Method_Cache::rewrite(Oop sel, Oop klass, int prim, fn_t primFunction, bool on_main) {
  FOR_EACH_PROBE(sel, klass, i, hash, e) {
    if (e->selector == sel  &&  e->klass == klass) {
      e->prim = prim;
      e->primFunction = primFunction;
      e->do_primitive_on_main = on_main;
      return;
    }
  }
}

bool Method_Cache::verify() {
  for (int i = 0;  i < Entries;  ++i) {
    entry* p = at(i);
    p->verify();
  }
  return true;
}

