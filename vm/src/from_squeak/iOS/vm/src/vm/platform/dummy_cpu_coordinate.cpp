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

Oop Dummy_CPU_Coordinate::get_stats() {
  
  int s = The_Squeak_Interpreter()->makeArrayStart();
  int mainRank = Logical_Core::main_rank;
  int mainX = Logical_Core::my_rank(), mainY = 0;
  PUSH_POSITIVE_32_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(mainX);
  PUSH_POSITIVE_32_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(mainY);
  PUSH_POSITIVE_32_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(mainRank);
  PUSH_POSITIVE_32_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(Logical_Core::group_size);
  PUSH_POSITIVE_32_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(1);
  PUSH_POSITIVE_32_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(Logical_Core::group_size);
  PUSH_POSITIVE_32_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(Logical_Core::remaining);
  
  Oop groupStats = The_Squeak_Interpreter()->makeArray(s);
  PUSH_WITH_STRING_FOR_MAKE_ARRAY(groupStats);
  
  return The_Squeak_Interpreter()->makeArray(s);
}
