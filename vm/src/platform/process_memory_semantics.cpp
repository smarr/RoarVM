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

# if Using_Processes

Logical_Core* Process_Memory_Semantics::_my_core = NULL;
int Process_Memory_Semantics::_my_rank = -1;
u_int64 Process_Memory_Semantics::_my_rank_mask = -1;


Memory_System _memory_system;
Squeak_Interpreter _interpreter;
Timeout_Timer_List_Head _timeout_head;

# endif

