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
   * This function returns the defined size or -1 when not yet initialized.
   */
  static int group_size() {
    if (globals == NULL)
      return -1;
    
    return globals->group_size;
  }
  
  /**
   * Returns the rank in the process group or -1 when not yet initialized.
   */
  static int process_rank() {
    return locals.rank;
  }
  
  /**
   * Returns the number of active members in the group or -1 when not yet
   * initialized.
   *
   * This number might not reflect the acurate number of running processes
   * since it is not updated when a process terminates abnormaly.
   */
  static int active_group_members() {
    if (globals == NULL)
      return -1;
    
    return globals->running_processes;
  }

private:
  static const char* Global_Shared_Mem_Name;
  
  // TODO: to support multiple RoarVM instances, this data structure
  //       needs to be a list of things instead of plain data
  class Globals {
  public:
    pid_t owning_process;
    
    /** The mutex is protecting the following fields and needs to be aquired
        to savely read/write them. */
    pthread_mutex_t mtx_rank_running;
    
    int   last_rank;
    int   running_processes;
    int   group_size;
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
  
  static Locals locals;
  
  static void register_process_and_determine_rank();
  
  static void unregister_and_clean_up();
};
