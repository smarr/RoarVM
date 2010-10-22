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




Oop Oop_Tracer::array_class() { return The_Squeak_Interpreter()->splObj(Special_Indices::ClassArray); }

void Oop_Tracer::do_all_roots(Oop_Closure* oc) {
  for (int i = 0;  i < end_of_live_data();  ++i)
    oc->value(&((Oop*)buffer)[i], NULL);
}

