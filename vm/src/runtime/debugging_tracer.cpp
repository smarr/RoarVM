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

bool Debugging_Tracer::force_real_context_allocation() {
  const int period = 1;
  if (!is_during_remote_context_allocation)
    return false;
  if (remote_context_allocation_requests % period != 0)
    return false;
  return true;
}

bool Debugging_Tracer::force_gc() {
  const int period = 2;
  if (!is_during_remote_context_allocation)
    return false;
  ++remote_context_allocations;
   if (remote_context_allocations % period  != 0)
    return false;
  lprintf( "forcing GC for debugging, %dth chance out of every %d\n",
          remote_context_allocation_requests, period);
  return true;
}

void Debugging_Tracer::record_gc() {
  ++gc_count;
  if (is_during_remote_context_allocation)
    lprintf( "GC during remote context allocation, %d RCA, %d gc\n",
            remote_context_allocation_requests, gc_count);
}

