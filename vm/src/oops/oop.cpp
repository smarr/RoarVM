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


void Oop::print(Printer* p) {
  if (is_int())
    p->printf("%d", integerValue());
  else
    as_object()->print(p);
}
void Oop::print_briefly(Printer* /* p */) {
  unimplemented();
}
void Oop::print_process_or_nil(Printer* p) {
  as_object()->print_process_or_nil(p);
}

bool Oop::isMemberOf(char* className) {
  return fetchClass().as_object()->className().as_object()->equals_string(className);
}

bool Oop::isKindOf(char* className) {
  Oop const nilObj = The_Squeak_Interpreter()->roots.nilObj;
  
  for (Oop klass = fetchClass();  klass != nilObj;  klass = klass.as_object()->superclass())
    if (klass.as_object()->className().as_object()->equals_string(className))
      return true;
  
  return false;
}

bool Oop::isKindOf(Oop aClass) {
  Oop const nilObj = The_Squeak_Interpreter()->roots.nilObj;
  
  for (Oop klass = fetchClass();  klass != nilObj;  klass = klass.as_object()->superclass())
    if (klass == aClass)
      return true;
  
  return false;
}

void Oop::test() { assert_always(sizeof(Oop) == 4); }

# if 0
Oop Oop::check_after_munging() {
  if (check_many_assertions) {
    verify_object();
  }
  return *this;
}

# endif
