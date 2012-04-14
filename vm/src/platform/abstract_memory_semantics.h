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


/**
 * The memory semantics are defined by the used concurrency abstraction
 * of the underlying system. It is distingushed between models,
 * which base on the assumption of shared memory as the default
 * and models which base on the notion of private memory is the default.
 *
 * This is based on 
 *   -     threads   (shared-by-default) 
 *   - and processes (private-by-default).
 */
class Abstract_Memory_Semantics {
public:
  static inline u_int64  my_rank_mask() { fatal(); return -1; }

  static inline void initialize_memory_system()       { fatal(); }
  static inline void initialize_local_memory_system() { fatal(); }
  
  static inline void initialize_interpreter()         { fatal(); }
  static inline void initialize_local_interpreter()   { fatal(); }
  
  static inline void initialize_timeout_timer()       { fatal(); }
  static inline void initialize_local_timeout_timer() { fatal(); }
  
  static void go_parallel(void (*)(/* helper_core_main */), char* /* argv */[]) { fatal (); };
  static inline int get_group_rank() { fatal(); return -1; }
  
  static inline void* shared_malloc(u_int32 /* sz */) { fatal(); return NULL; }
  static inline void* shared_calloc(u_int32 /* num_members */, u_int32 /* mem_size */) { fatal(); return NULL; }
  static inline void  shared_free(void*) { fatal(); }
  
  static inline bool is_using_threads() { fatal(); return 0; }
  
};
