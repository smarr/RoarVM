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


# if Multiple_Tileras


#include "headers.h"

void Chip_to_Chip_Zero_Copy_Command_Receiver::check_completion() {
  Chip_to_Chip_Zero_Copy_Command_Queue_Endpoint::check_completion();
  
  assert_always(completion.len <= request.len);
  flags_should_be(0, "recv");
}


# endif // Multiple_Tileras
