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


# if Use_ReadMostly_Heap
  class   Read_Mostly_Memory_System;
  typedef Read_Mostly_Memory_System Memory_System;
  /* We define the number of partions here to avoid cyclic dependencies.
     For the read-mostly heap, it is used for the different mutabilities. */
  # define Memory_System_Partitions 2
# else
  class   Basic_Memory_System;
  typedef Basic_Memory_System Memory_System;
  /* Defined here to avoid cyclic dependencies. */
  # define Memory_System_Partitions 1
# endif
