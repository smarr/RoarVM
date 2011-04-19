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

# if Profile_Image

#include  <string>
#include  <map>


void Profiling_Tracer::trace(Squeak_Interpreter* si) {
  Execution_Tracer::trace(si);

  record_for_profile(si->method(), si->roots.receiver, si->activeContext_obj()->is_this_context_a_block_context());
}

void get_profile(std::map<std::string, int>** prof) {
  static std::map<std::string, int> profile;
#warning Stefan: warning, not threadsafe

  *prof = &profile;
}


void Profiling_Tracer::record_for_profile(Oop meth, Oop rcvr, bool is_block) {
  if (Logical_Core::num_cores > 1)
    return;

  static std::string lastSignature;
  std::map<std::string, int>* profile;
#warning Stefan: warning, not threadsafe
  get_profile(&profile);

  static char buff[2024];
  StringPrinter p(buff, 2023);

  Oop klass = rcvr.fetchClass();
  Oop sel, mclass;
  bool have_sel_and_mclass = klass.as_object()->selector_and_class_of_method_in_me_or_ancestors(meth, &sel, &mclass);
  if (!have_sel_and_mclass) return;
  if (mclass == The_Squeak_Interpreter()->roots.nilObj)
    mclass = rcvr.fetchClass();

  rcvr.print(&p);
  if (mclass != klass) {
    p.printf("(");
    bool is_meta;
    Oop mclassName = mclass.as_object()->name_of_class_or_metaclass(&is_meta);
    if (is_meta) p.printf("class ");
    mclassName.as_object()->print_bytes(&p);
    p.printf(")");
  }
  p.printf(">>");
  sel.as_object()->print_bytes(&p);
  if (is_block) p.printf("[]");

  buff[2023] = 0;
  std::string signature(buff);

  if (lastSignature != signature) {
    lastSignature = signature;
    ++(*profile)[lastSignature];
  }


  // STEFAN: a little hack to be able to get the printout during debugging, any better ways to do so?
  static bool print_profile = false;
  if (print_profile) {
    this->print_profile(debug_printer);
  }
}

void Profiling_Tracer::print() {
  Execution_Tracer::print();

  if (Logical_Core::num_cores > 1)
    return;


  std::map<std::string, int>* profile;
  get_profile(&profile);

  for(std::map<std::string, int>::const_iterator it = profile->begin();
      it != profile->end();
      ++it) {
    debug_printer->printf(it->first.c_str());
    debug_printer->printf(",");
    debug_printer->printf("%d", it->second);
    debug_printer->printf("\n");
  }
}

# endif

