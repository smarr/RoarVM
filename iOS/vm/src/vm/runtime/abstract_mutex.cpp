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

void trace_mutex_evt(int x, const char* why) {
  if (Trace_Execution  &&  The_Squeak_Interpreter()->execution_tracer() != NULL)
    The_Squeak_Interpreter()->execution_tracer()->set_aux(x, why);
  //lprintf( "trace_mutex_evt %d\n", x);
}

