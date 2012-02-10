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
const char* Shared_Region_Name = "/POSIX_Processes-SR-%lu";

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
  
  if (   globals->owning_process != locals().parent
      && globals->owning_process != locals().pid) {
    warnx("There is some confusion with global data, which is shared system"
          " wide. Currently there is no support for more then one instance "
          " of a program using this library!");
    OS_Interface::mutex_destruct(&globals->mtx_rank_running);
    initialize_processes_globals();
  }
  
  register_process_and_determine_rank();
  
  if (locals().rank != 0)
    map_shared_regions();
  
# warning TODO: add pinning for processes to cores here!!! STEFAN
  
  return 0;
}

void POSIX_Processes::initialize_processes_globals() {
  assert(globals != NULL);
  
  // Just to be sure, not strictly necessary
  memset(globals, 0, sizeof(Globals));
  
  // basic infos
  globals->last_rank         = 0;
  globals->running_processes = 1;
  globals->group_size        = 1;
  globals->owning_process    = getpid();
  
  // the cross-process mutex
  OS_Interface::mutex_init_for_cross_process_use(&globals->mtx_rank_running);
}

void POSIX_Processes::register_process_and_determine_rank() {
  OS_Interface::mutex_lock(&globals->mtx_rank_running);
  
  if (locals().pid == globals->owning_process) {
    locals().rank = 0;
  }
  else {
    globals->last_rank++;
    locals().rank = globals->last_rank;
    
    globals->running_processes++;
  }
  
  OS_Interface::mutex_unlock(&globals->mtx_rank_running);
  
  atexit(unregister_and_clean_up);
}

bool POSIX_Processes::unregister_in_global_memory() {
  OS_Interface::mutex_lock(&globals->mtx_rank_running);
  
  globals->running_processes--;
  const bool last_process = (0 == globals->running_processes);
  
  OS_Interface::mutex_unlock(&globals->mtx_rank_running);
  
  if (last_process)
    OS_Interface::mutex_destruct(&globals->mtx_rank_running);    

  return last_process;
}

void POSIX_Processes::unregister_and_clean_up() {
  Globals globals_copy = *globals;
  const bool last_process = unregister_in_global_memory();
  
  
  for (size_t i = 0; i < num_of_shared_mmap_regions; i++) {
    if (globals_copy.shared_mmap_regions[i].base_address) {
      /* first unmap */
      munmap(globals_copy.shared_mmap_regions[i].base_address,
             globals_copy.shared_mmap_regions[i].len);
      
      /* then remove the shm object */
      if (last_process) {
        char region_name[BUFSIZ] = { 0 };
        snprintf(region_name, sizeof(region_name), Shared_Region_Name, i);
        shm_unlink(region_name);
      }
    }
  }
  
  munmap(globals, sizeof(Globals));
  
  if (last_process)
    shm_unlink(Global_Shared_Mem_Name);
}

int POSIX_Processes::start_group(size_t num_processes, char** argv) {
  OS_Interface::mutex_lock(&globals->mtx_rank_running);  
  globals->group_size = num_processes;
  OS_Interface::mutex_unlock(&globals->mtx_rank_running);
  
  for (size_t i = 1; i < num_processes; i++) {
    pid_t pid = fork(); //or vfork for performance
    
    if (0 == pid) {
      // child
      execv(argv[0], argv);
    }
    else if (pid > 0) {
      // master: just continue with the loop
      continue;
    }
    else {
      // error
      // TODO: do it properly
      perror("Failure on forking child processes.");
      return -1;
    }
  }

  // With the iLib, the main process would also get replaced and start over
  // but that caused more problems with debugging, and currently it does not 
  // seem to have obvious drawbacks. So, no execv here.
  // execv(argv[0], argv);
  return 0;
}

void* POSIX_Processes::request_globally_mmapped_region(size_t id, size_t len) {
  assert_always(id < num_of_shared_mmap_regions);
  
  if (globals->shared_mmap_regions[id].base_address) {
    assert_message(globals->shared_mmap_regions[id].len == len,
                   "The requested region was already allocated, but the size did not match.");
    return globals->shared_mmap_regions[id].base_address;
  }
  
  char region_name[BUFSIZ] = { 0 };
  snprintf(region_name, sizeof(region_name), Shared_Region_Name, id);
  shm_unlink(region_name);
  int shared_fd = shm_open(region_name, O_RDWR  | O_CREAT, S_IRUSR | S_IWUSR);
  
  if (shared_fd < 0) {
    // TODO: do it properly
    perror("Could not create shared memory object for a mmapped memory region.");
    return NULL;
  }
  
  if (-1 == ftruncate(shared_fd, len)) {
    // TODO: do it properly
    perror("The shared memory object (mmapped region) could"
           " not be set to the requested size");
    return NULL;
  }

  assert(shared_fd >= 0);
  
  int mmap_prot  = PROT_READ | PROT_WRITE;
  int mmap_flags = MAP_SHARED; // STEFAN: do we need a special flag to enable semaphores in this memory? MAP_HASSEMAPHORE?
  int mmap_offset= 0;
  
  void* result = mmap(NULL, len, mmap_prot, mmap_flags, shared_fd, mmap_offset);
  if (MAP_FAILED == result) {
    // TODO: do it properly
    perror("Could not establish memory mapping for the globally shared"
           " memory object.");
    return NULL;
  }

  globals->shared_mmap_regions[id].set(result, len, mmap_prot, mmap_flags, mmap_offset);
  return result;
}

void POSIX_Processes::map_shared_regions() {
  for (size_t i = 0; i < num_of_shared_mmap_regions; i++) {
    if (!globals->shared_mmap_regions[i].base_address)
      continue;
    
    char region_name[BUFSIZ] = { 0 };
    snprintf(region_name, sizeof(region_name), Shared_Region_Name, i);
    int shared_fd = shm_open(region_name, O_RDWR, S_IRUSR | S_IWUSR);
    
    if (shared_fd < 0) {
      // TODO: do it properly
      perror("Could not create shared memory object for a mmapped memory region.");
      return;
    }
    assert(shared_fd >= 0);
    
    void* result = mmap(globals->shared_mmap_regions[i].base_address,
                        globals->shared_mmap_regions[i].len,
                        globals->shared_mmap_regions[i].prot,
                        globals->shared_mmap_regions[i].flags,
                        shared_fd,
                        globals->shared_mmap_regions[i].offset);
    
    if (MAP_FAILED == result) {
      // TODO: do it properly
      perror("Could not establish memory mapping for the globally shared"
             " memory object.");
      return;
    }
  }
}

