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


class Chip_to_Chip_Zero_Copy_Command_Receiver: public Chip_to_Chip_Zero_Copy_Command_Queue_Endpoint {
public:
  bool is_sender() { return false; }
  
private:
  int request_flags() { return TILEPCI_RCV_MUST_EOP; }
  int request_cookie() { return test_cookie + 2; }
  void check_completion();
};



# endif // Multiple_Tileras
