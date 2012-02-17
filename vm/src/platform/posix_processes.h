/******************************************************************************
 *  Copyright (c) 2008 - 2011 IBM Corporation and others.
 *  All rights reserved. This program and the accompanying materials
 *  are made available under the terms of the Eclipse Public License v1.0
 *  which accompanies this distribution, and is available at
 *  http://www.eclipse.org/legal/epl-v10.html
 * 
 *  Contributors:
 *    Stefan ;, Vrije Universiteit Brussel - Initial Implementation
 ******************************************************************************/


# include <pthread.h>

/**
 * The class provides the functionality to start up a group of processes
 * to run the same exectuable together to form a single virtual machine
 * instance.
 */
class POSIX_Processes {
public:
  /** 
   * This class/library requires global data structures outside
   * a single process's scope which need to be initialized properly.
   */
  static int initialize();
  
  /**
   * Create a group of processes based on the given parameters.
   * The current process will be replaced and the given executable will
   * be executed instead.
   * Return on error with -1, does NOT return on success.
   */
  static int start_group(size_t num_processes, char** argv);
  
  /**
   * A process group is statically defined with start_group(...) which
   * includes its size.
   * This function returns the defined size or 1 when not yet initialized.
   */
  static int group_size() {
    if (globals == NULL)
      return 1;
    
    return globals->group_size;
  }
  
  /**
   * Returns the rank in the process group or -1 when not yet initialized.
   */
  static int process_rank() {
    return locals().rank;
  }
  
  /**
   * Returns the number of active members in the group or 1 when not yet
   * initialized.
   *
   * This number might not reflect the acurate number of running processes
   * since it is not updated when a process terminates abnormaly.
   */
  static int active_group_members() {
    if (globals == NULL)
      return 1;
    
    return globals->running_processes;
  }
  
private:
  static const char* Global_Shared_Mem_Name;
  static const size_t num_of_shared_mmap_regions = 16;
  
  class Shared_MMAP_Region {
  public:
    Shared_MMAP_Region()
      : base_address(NULL), len(0), prot(0),
        flags(0), offset(0) {}
    
    void*  base_address;
    size_t len;
    int    prot;
    int    flags;
    off_t  offset;
    
    void set(void*  base_address, size_t len, int prot, int flags, off_t offset) {
      this->base_address = base_address;
      this->len    = len;
      this->prot   = prot;
      this->flags  = flags;
      this->offset = offset;
    }

    void reset() {
      base_address = NULL;
      len    = 0;
      prot   = flags = 0;
      offset = 0;
    }
  };
  
  // TODO: to support multiple RoarVM instances, this data structure
  //       needs to be a list of things instead of plain data
  class Globals {
  public:
    pid_t owning_process;
    
    /** The mutex is protecting the following fields and needs to be aquired
        to savely read/write them. */
    OS_Interface::Mutex mtx_rank_running;
    
    int   last_rank;
    int   running_processes;
    int   group_size;
    Shared_MMAP_Region shared_mmap_regions[num_of_shared_mmap_regions];
    
    /** Record all involved process ids */
    pid_t processes[Max_Number_Of_Cores];
  };
  
  /**
   * A set of interprocess globals required for this library.
   */
  static Globals* globals;
  
  static void initialize_processes_globals();
  
  class Locals {
  public:
    Locals() : parent(getppid()), pid(getpid()), rank(-1) {}
    
    const pid_t parent;
    const pid_t pid;
    int rank;
  };
  
  static Locals& locals() {
    static Locals _locals = Locals();
    return _locals;
  }
  
  static void register_process_and_determine_rank();
  
  static void unregister_and_clean_up();
  static bool unregister_in_global_memory();
  static void map_shared_regions();
  
  static void initialize_termination_handler();

public:

  /**
   * Request mmapped memory that is globally available for all processes.
   * Each process is mapping the memory to the same address, using the same
   * memory protection flags, etc. This allows to use the memory as in any
   * other shared memory setup with 'portable' pointers.
   */
  static void* request_globally_mmapped_region(size_t id, size_t len);
  
  /**
   * Get the pointer to an already allocated memory region.
   */
  static void* get_globally_mmapped_region_by_id(size_t id) {
    assert_always(id < num_of_shared_mmap_regions);
    return globals->shared_mmap_regions[id].base_address;
  }
  
  static bool is_owner_process() {
    if (Using_Threads)
      return true;
    if (globals == NULL)
      return false; // don't know
    return globals->owning_process == locals().pid;
  }

  static void unregister_child_termination_handler();
  static void print_globals();
};
