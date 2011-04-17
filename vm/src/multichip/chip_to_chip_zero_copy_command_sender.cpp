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

void Chip_to_Chip_Zero_Copy_Command_Sender::check_completion() {
  Chip_to_Chip_Zero_Copy_Command_Queue_Endpoint::check_completion();
  
  assert_always_eq(completion.len, request.len);
  
  if (completion.flags & TILEPCI_CPL_LINK_DOWN) {
    static bool kvetched = false;
    if (!kvetched) {
      lprintf("discarding TILEPCI_CPL_LINK_DOWN, no more warnings\n");
      kvetched = true;
    }
  }
  completion.flags &= ~TILEPCI_CPL_LINK_DOWN;
  flags_should_be(TILEPCI_CPL_EOP, "send");
}


# endif // Multiple_Tileras
