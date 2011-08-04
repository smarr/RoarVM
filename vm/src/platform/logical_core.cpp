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

int Logical_Core::num_cores     =  1;
int Logical_Core::main_rank     =  0;

int Logical_Core::group_size    = -1;
int Logical_Core::remaining     = -1;


void Logical_Core::initialize_all_cores() {
  logical_cores = new Logical_Core[num_cores];

  for (size_t i = 0;  i < size_t(num_cores);  ++i)
    logical_cores[i].initialize(i);
}
