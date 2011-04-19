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


class Interactions {
public:
  const static bool verbose = false;

  int     remote_prim_count;
  u_int64 remote_prim_cycles;

  Interactions() {
    remote_prim_count  = 0;
    remote_prim_cycles = 0LL;
  }

  Oop get_stats();

  // xxxxxxx reify interactions and make these funs those
  // Explanation:
  // It would probably be better to reify the interactions and I have tried this with one or two.
  // Never finished the experiment. -- dmu 4/09
  void recycleContextIfPossible(int dst, Oop, const int current_rank);
  Object* add_object_from_snapshot_allocating_chunk(int dst, Oop, Object*);
  void do_all_roots_here(Oop_Closure*);
  void run_primitive(int dst, fn_t f);
  fn_t load_function_from_plugin(int dst, const char* fn, const char* plugin);

  void get_screen_info(int*, int*);
  bool getNextEvent_on_main(int*);
  // no faster void init_ctx(Object*, Oop, Oop, int, Oop, int, Oop*);
  Oop sample_each_core(int what_to_sample);

};

extern Interactions The_Interactions;

