/******************************************************************************
 *  Copyright (c) 2008 - 2010 IBM Corporation and others.
 *  All rights reserved. This program and the accompanying materials
 *  are made available under the terms of the Eclipse Public License v1.0
 *  which accompanies this distribution, and is available at
 *  http://www.eclipse.org/legal/epl-v10.html
 * 
 *  Contributors:
 *    Stefan Marr, Vrije Universiteit Brussel - Initial Implementation
 ******************************************************************************/


void* POSIX_OS_Interface::rvm_malloc_shared(size_t sz) {
  return shared_memory_allocator()->allocate(sz);
}

void* POSIX_OS_Interface::rvm_calloc_shared(size_t num_members, size_t mem_size)  {
  return shared_memory_allocator()->allocate_elements(num_members, mem_size);
}

void POSIX_OS_Interface::rvm_free_shared(void* mem) {
  shared_memory_allocator()->free(mem);
}

inline void POSIX_OS_Interface::initialize() {
  static bool initialized = false;
  if (Using_Processes && !initialized) {
    POSIX_Processes::initialize();
    initialized = true;
  }
}

inline int POSIX_OS_Interface::get_process_rank() { 
  return POSIX_Processes::process_rank();
}
