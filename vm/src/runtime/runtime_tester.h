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


class Runtime_Tester {
 public:
  static void test() {
    Bitmap::test();
    Bytemap::test();
    typedefs::check_typedefs();
    Object::test();
    Oop::test();
    Rank_Set::unit_test();
    Safepoint_Request_Queue::unit_test();
  }
};

