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


class Logical_Core; // forward declaration to resolve cyclic dependency

class Abstract_Message_Queue {
public:
  void send_message(abstractMessage_class*) { fatal(); };
  
  void buffered_send_buffer(void*, int) { fatal(); };
  static void* buffered_receive_from_anywhere(bool /* wait */, Logical_Core** /* buffer_owner */, Logical_Core* const /* receiver */) { fatal(); return NULL; };
  void release_oldest_buffer(void*) { fatal(); };
  
  /**
   * Depending on the platform, this might not be efficiently implementable.
   *
   * If it is not implemented, the latency to notice an incoming message
   * increases. However, it seems to be not a problem, in the sense that
   * the VM works on Tilera where it is not implemented at the moment.
   *                                                         (Stefan 2010-08-02)
   */
  static bool are_data_available(Logical_Core* const) { return false; };
};

