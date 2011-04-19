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


# if Profile_Image

class Profiling_Tracer: public Execution_Tracer {
public:
  Profiling_Tracer(int n): Execution_Tracer(n) {};

  void record_for_profile(Oop method, Oop rcvr, bool is_block);

  void print();

  void trace(Squeak_Interpreter* si);

  static void print_entries(Oop, Printer*);
  static void print_profile(Printer*);
};

# endif

