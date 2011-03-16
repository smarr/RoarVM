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


struct Deferred_Request {
  static inline Deferred_Request** first_TL() { return &first_request[rank_on_threads_or_zero_on_processes()]; }
  static Deferred_Request* first_request[Memory_Semantics::max_num_threads_on_threads_or_1_on_processes];
  Deferred_Request* next;
  abstractMessage_class* request;
  
  Deferred_Request(abstractMessage_class*);
  static void add(abstractMessage_class*);
  static void service_and_free_all();
  void service();
  ~Deferred_Request() { free(request); }
  static void do_all_roots(Oop_Closure*);
  static void print_all_messages(int);
  
  void print() { request->print(); }
};
