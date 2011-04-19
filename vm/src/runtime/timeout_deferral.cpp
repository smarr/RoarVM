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


Timeout_Deferral:: Timeout_Deferral() {
  ++The_Squeak_Interpreter()->timeout_deferral_counters[Logical_Core::my_rank()];
}


Timeout_Deferral::~Timeout_Deferral() {
  if (--The_Squeak_Interpreter()->timeout_deferral_counters[Logical_Core::my_rank()] == 0)
    Timeout_Timer::restart_all();
}


bool Timeout_Deferral::are_timeouts_deferred() {
  for (int i = 0;  i < Max_Number_Of_Cores;  ++i)
    if (The_Squeak_Interpreter()->timeout_deferral_counters[i]) return true;
  return false;
}

