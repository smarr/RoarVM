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
POSIX_Processes::Locals   POSIX_Processes::locals;

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
  
  if (   globals->owning_process != locals.parent
      && globals->owning_process != locals.pid) {
    warnx("There is some confusion with global data, which is shared system"
          " wide. Currently there is no support for more then one instance "
          " of a program using this library!");
    pthread_mutex_destroy(&globals->mtx_rank_running);
    initialize_processes_globals();
  }
  
  register_process_and_determine_rank();
  
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
  
  if (locals.pid == globals->owning_process) {
    locals.rank = 0;
  }
  else {
    globals->last_rank++;
    locals.rank = globals->last_rank;
    
    globals->running_processes++;
  }
  
  OS_Interface::mutex_unlock(&globals->mtx_rank_running);
  
  atexit(unregister_and_clean_up);
}

void POSIX_Processes::unregister_and_clean_up() {
  OS_Interface::mutex_lock(&globals->mtx_rank_running);
  
  globals->running_processes--;
  const bool last_process = (0 == globals->running_processes);
  
  OS_Interface::mutex_unlock(&globals->mtx_rank_running);
  
  if (last_process) {
    OS_Interface::mutex_destruct(&globals->mtx_rank_running);
  }
  
  munmap(globals, sizeof(Globals));
  
  if (last_process) {
    shm_unlink(Global_Shared_Mem_Name);
  }
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
    }
  }

  // All child processes initialized, lets start over
  // this process, too.
  // Mostly for consistency with other platforms and libraries.
  execv(argv[0], argv);
  return -1;
}
