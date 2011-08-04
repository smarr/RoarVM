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


# include "headers.h"

Logical_Core* logical_cores;
Logical_Core gc_core;

int Logical_Core::num_cores  =  1;
int Logical_Core::main_rank  =  0;

int Logical_Core::group_size = -1;
int Logical_Core::remaining  = -1;


void Logical_Core::initialize_all_cores() {
  logical_cores = new Logical_Core[num_cores];

  for (size_t i = 0;  i < size_t(num_cores);  ++i)
    logical_cores[i].initialize(i);
}

void Logical_Core::initialize_GC_core(){
    // Cores are nubered from 0-groupsize -> group_size is a free rank.
    // But not yet set at this point so use num_cores
    gc_core.initialize(num_cores);
}

void Logical_Core::start_GC_thread(){
    // Create an instance of GC_thread_Class and start (spawn+run)
    // The instance is also stored statically in GC_Thread_Class. 
    GC_Thread_Class gc_thread(gc_core);
    gc_thread.start();
}
