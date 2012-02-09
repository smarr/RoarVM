/******************************************************************************
 *  Copyright (c) 2008 - 2010 IBM Corporation and others.
 *  All rights reserved. This program and the accompanying materials
 *  are made available under the terms of the Eclipse Public License v1.0
 *  which accompanies this distribution, and is available at
 *  http://www.eclipse.org/legal/epl-v10.html
 * 
 *  Contributors:
 *    Stefan Marr, Vrije Universiteit Brussel - Initial Implementation
 *
 ******************************************************************************/


# include "headers.h"

void BufferedChannelDebug::send(const void* data, size_t size) {
  void* const buffer = Memory_Semantics::shared_malloc(size);
  memcpy(buffer, data, size);
  
  OS_Interface::mutex_lock(&lock);
  channel.enqueue(buffer);
  OS_Interface::mutex_unlock(&lock);
}

void BufferedChannelDebug::releaseOldest(void* const buffer) const {
  Memory_Semantics::shared_free(buffer);
}
