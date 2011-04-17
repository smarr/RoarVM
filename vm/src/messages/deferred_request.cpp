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


Deferred_Request* Deferred_Request::first_request[Memory_Semantics::max_num_threads_on_threads_or_1_on_processes] = { NULL };

void Deferred_Request::add(abstractMessage_class* m) {
  new Deferred_Request(m);
}

Deferred_Request::Deferred_Request(abstractMessage_class* m) {
  next = *first_TL();
  *first_TL() = this;
  
  int sz = m->size_for_transmission_and_copying();
  request = (abstractMessage_class*)malloc(sz);
  memcpy(request, m, sz);
}


void Deferred_Request::service_and_free_all() {
  for (Deferred_Request* req = *first_TL();  req != NULL;  req = *first_TL()) {
    req->service();
    *first_TL() = req->next;
    delete req;
  }
}


void Deferred_Request::service() {
  // use a switch statement to avoid a lot of virtual dispatches
  switch (request->header) {
    default: fatal("what?"); break;
    # define MAKE_CASE(name, superclass, constructor_formals, superconstructor_actuals, constructor_body, class_body, ack_setting, safepoint_delay_setting) \
      case Message_Statics::name: ((name##_class*)request)->handle_me_or_maybe_delay(); break; \

      FOR_ALL_MESSAGES_DO(MAKE_CASE)
    # undef MAKE_CASE
  }
}


void Deferred_Request::do_all_roots(Oop_Closure* oc) {
  for (Deferred_Request* r = *first_TL();  r != NULL;  r = r->next)
    r->request->do_all_roots(oc);
}

void Deferred_Request::print_all_messages(int rank) {
  if (  sizeof(first_request) / sizeof(first_request[0])    == 1  &&  Logical_Core::num_cores > 1)
    fatal("cannot possibly work, need shared memory");
  for (Deferred_Request* r = first_request[rank];  r != NULL;  r = r->next) {
    lprintf("printing 0x%x\n", r);
    r->print();
  }
}
