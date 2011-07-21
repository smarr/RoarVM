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


# include "headers.h"

const char* POSIX_Processes::Global_Shared_Mem_Name = "/POSIX_Processes_cpp-001";

POSIX_Processes::Globals* POSIX_Processes::globals = NULL;

int POSIX_Processes::initialize() {
  bool initialize_globally = false;
  
  int shared_fd = shm_open(Global_Shared_Mem_Name,
                           O_RDWR,
                           S_IRUSR | S_IWUSR);
  
  if (shared_fd < 0) {
    switch (errno) {
      case ENOENT:
        // shared memory object does not exist, need to create it and
        // late initialize it properly
        initialize_globally = true;
        
        shared_fd = shm_open(Global_Shared_Mem_Name,
                             O_RDWR  | O_CREAT,
                             S_IRUSR | S_IWUSR);
        
        if (shared_fd < 0) {
          // TODO: do it properly
          perror("Could not create GLOBAL_SHARED_MEM_NAME shared"
                 " memory object");
          return -1;
        }
        
        if (-1 == ftruncate(shared_fd, sizeof(Globals))) {
          // TODO: do it properly
          perror("The shared memory object (Global_Shared_Mem_Name) could"
                 " not be set to the required size");
          return -1;
        }
        break;
        
      default:
        // TODO: do it properly
        perror("POSIX_Processes::initilize failed on accessing the globally"
               " shared memory object.");
        return -1;
    }
  }
  
  assert(shared_fd >= 0);
  
  globals = (Globals*)mmap(NULL, sizeof(Globals),
                          PROT_READ | PROT_WRITE,
                          MAP_SHARED,
                          shared_fd, 0);
  if (MAP_FAILED == globals) {
    // TODO: do it properly
    perror("Could not establish memory mapping for the globally shared"
           " memory object.");
    return -1;
  }
  
  if (initialize_globally) {
    initialize_processes_globals();
  }
  
  initialize_process_local_data();
  
# warning TODO: add pinning for processes to cores here!!! STEFAN
  
  return 0;
}
