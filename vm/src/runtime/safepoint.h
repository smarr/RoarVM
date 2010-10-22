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


class Safepoint_Actions {
  public:
  static void acquire_action(const char*);
  static void release_action(const char*);
  static bool is_initialized();
  static OS_Mutex_Interface* get_mutex();
  static bool is_held();
};

// typedef Abstract_Mutex<Safepoint_Actions> Safepoint;

// NOTE: Safepoint_for_moving_objects means I may do a GC, or move objects. In other words, change the locations of objects, or change the allocation pointers.


Define_RVM_Mutex(Safepoint_for_moving_objects, Safepoint_Actions,17,18)


class Safepoint_Tracker {
  int    _spin_depth;
  bool  _is_every_other_core_safe;
  int   _sequence_number_of_last_granted_safepoint;
  bool  _does_another_core_need_me_to_spin;
  int   _which_other_core_needs_me_to_spin;
  int   _seq_no_of_another_needs_me_to_spin; 
  const char* _why_another_core_needs_me_to_spin;
  int   _am_requesting_other_cores_to_safepoint;

  public:
  Safepoint_Acquisition_Timer acquisition_timer;
  static const bool verbose = false;


 public:
  Safepoint_Tracker() : acquisition_timer() {
    _spin_depth = 0;
    _is_every_other_core_safe = false;
    _sequence_number_of_last_granted_safepoint = 0;
    _does_another_core_need_me_to_spin = false;
    _which_other_core_needs_me_to_spin = -1;
    _seq_no_of_another_needs_me_to_spin = -1;
    _why_another_core_needs_me_to_spin = "";
    assert(acquisition_timer.is_on_list());
    _am_requesting_other_cores_to_safepoint = 0;
  }

  void request_other_cores_to_safepoint(const char*);
# define spin_if_safepoint_requested() spin_if_safepoint_requested_with_arguments(__FUNCTION__, __FILE__, __LINE__ )
  void spin_if_safepoint_requested_with_arguments(const char*, const char*, int);

  void release_other_cores_from_safepoint(const char*);

  bool is_every_other_core_safe() { return _is_every_other_core_safe; }
  void every_other_core_is_safe(int seq_no)  {
    if (verbose) lprintf("every_other_core_is_safe()\n");
    _is_every_other_core_safe = true;
    _sequence_number_of_last_granted_safepoint = seq_no;
  }
  void every_other_core_no_longer_safe() {
    if (verbose) lprintf("every_other_core_no_longer_safe()\n");
    _is_every_other_core_safe = false;
  }

  bool does_another_core_need_me_to_spin() { return _does_another_core_need_me_to_spin; }

  int which_other_core_needs_me_to_spin() { return _which_other_core_needs_me_to_spin; }
  const char* why_other_core_needs_me_to_spin() { return _why_another_core_needs_me_to_spin; }

  void another_core_needs_me_to_spin(int for_whom, int, const char* why);
  void another_core_no_longer_needs_me_to_spin(int);
  int spin_depth() { return _spin_depth; }
  bool am_spinning() { return spin_depth() > 0; }
  bool am_trying_to_acquire_safepoint() { return _am_requesting_other_cores_to_safepoint > 0; }
  
  bool have_acquired_safepoint() {  return is_every_other_core_safe(); }
  bool am_acquiring_safepoint_while_spinning_for(int r) {
    return am_trying_to_acquire_safepoint() && am_spinning() && which_other_core_needs_me_to_spin() == r;
  }

  void self_destruct_all();


  private:
  void spin_in_safepoint(const char*, const char*, int);
  void tell_core_I_am_spinning(int seq_no_of_request, bool was_spinning);
  void print_msg_for_request_safepoint(const char* msg, const char* why);

};


class Safepoint_Master_Control {
  Safepoint_Request_Queue cores_asking_for_a_global_safepoint;
  Rank_Set outstanding_spin_requests;
  Rank_Set spinners;
  Timeout_Timer* spin_request_timers[Max_Number_Of_Cores]; // xxxxxx These timers help debugging but may slow us down. -- dmu 4/09
  Rank_Set all_cores;
  static const int none = -1;
  int     core_holding_global_safepoint;
  static int request_depth; // top-level request recursion, threadsafe?: not critical, does no have a functional purpose, Stefan 2009-09-06
  int step_recurse_level; // redundant with request_depth
  
  // For debugging checks that might as well be left in the code, since safepoints are expensive anyway -- dmu 5/21/10
  int*    prior_outstanding_spin_requests_sequence_numbers;
  int*    outstanding_spin_requests_sequence_numbers;
  int*    spinners_sequence_numbers; 
  int     current_safepoint_sequence_number; 
  int     global_safepoint_request_sequence_number;

public:
  static const bool verbose = false;
  void smc_printf(const char*, ...);
  void smc_vprintf(const char*, va_list);
  static void smc_white_space();

  Safepoint_Master_Control() :
    cores_asking_for_a_global_safepoint(Max_Number_Of_Cores),
    outstanding_spin_requests(), spinners(), all_cores() {
    core_holding_global_safepoint = none;
    all_cores = Rank_Set::all_up_to(Logical_Core::group_size);
    FOR_ALL_RANKS(r)
      spin_request_timers[r] = new Timeout_Timer("spin request", 45 /* shorter than default */, r);
    step_recurse_level = 0;
      
    current_safepoint_sequence_number = -1;
    global_safepoint_request_sequence_number = 0;

    prior_outstanding_spin_requests_sequence_numbers = new int[Max_Number_Of_Cores];
          outstanding_spin_requests_sequence_numbers = new int[Max_Number_Of_Cores];
 
    spinners_sequence_numbers = new int[Max_Number_Of_Cores];
    for (int i = 0;  i < Max_Number_Of_Cores; ++i) 
      prior_outstanding_spin_requests_sequence_numbers[i] = outstanding_spin_requests_sequence_numbers[i] = spinners_sequence_numbers[i] = -1;
  }


  void request_other_cores_to_safepoint(int, const char* why);
  void release_other_cores_from_safepoint(int);
  void a_core_is_now_safe(int, int, bool);

private:

  void run();
  bool step();
  bool step_towards_granting();
  bool maybe_stop_a_spinner();
  bool maybe_cancel_a_spin_request();
  bool maybe_make_a_spin_request_to_another_core(Rank_Set);
  bool maybe_ask_myself_to_spin();

  bool maybe_grant_safepoint_to_next_requester(Rank_Set);


  bool maybe_release_a_spinner();

  void request_core_to_spin(int, int, int, const char*);
  void tell_core_to_stop_spinning(int, int);
  void grant_safepoint_to_next_asker();

  int         next_grantee()      { return cores_asking_for_a_global_safepoint.oldest_rank(); }
  int         next_sequence_number() { return cores_asking_for_a_global_safepoint.oldest_sequence_number(); }
  const char* next_grantee_why()  { return cores_asking_for_a_global_safepoint.oldest_why();  }

  void print_string(char* buf, int buf_size);
  struct Top_Level {
    Top_Level() { ++request_depth; }
    ~Top_Level() { --request_depth; smc_white_space(); }
  };

};

