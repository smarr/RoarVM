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


class Scheduler_Mutex_Actions {
  static const bool tracking = false;
public:
  static void acquire_action(Squeak_Interpreter*, const char*);
  static void release_action(Squeak_Interpreter*, const char*);
  static bool is_initialized(Squeak_Interpreter*);

  static OS_Mutex_Interface* get_mutex(Squeak_Interpreter*);
  static bool is_held(Squeak_Interpreter*);
};


/** Macro is not used, as it would be the convention, since we need to extend
    the behavior of the Scheduler_Mutex_Actions */
//Define_RVM_Mutex(Scheduler_Mutex, Scheduler_Mutex_Actions,13,14)



class Scheduler_Mutex {
private:
  /* globally unique ids, like in Define_RVM_Mutex */
  static const int acquire_ID = 13;
  static const int release_ID = 14;
  
  Safepoint_Ability sa;
  friend class Scheduler_Mutex_Actions;
  
  const char* why;
  Squeak_Interpreter* const interpreter;
  
public:
  static bool is_held_for_interpreter(Squeak_Interpreter* const interp)  { 
    return Scheduler_Mutex_Actions::is_held(interp);
  }
  static void assert_held_for_interpreter(Squeak_Interpreter* const interp)  { 
    assert(!Scheduler_Mutex_Actions::is_initialized(interp) 
           || is_held_for_interpreter(interp)); 
  }
  
  
  Scheduler_Mutex(const char* w = "", 
                  Squeak_Interpreter* interp = NULL) 
  /* must be true while waiting to acquire, otherwise could deadlock */
  : sa(Safepoint_Ability::is_interpreter_able()), interpreter(interp) {
    assert(interp != NULL);
    why = w;
    u_int64 start = OS_Interface::get_cycle_count();
    if (acquire_ID) trace_mutex_evt(acquire_ID, why);
    
    OS_Mutex_Interface* mutex = Scheduler_Mutex_Actions::get_mutex(interpreter);
    /* Tricky; some mutexes may recurse since they have a receive_and_handle_one_message.
     If so, need to acquire mutex even though are recursing, so delay increment of recursion_count 
     However, clients of this macro must also manipulate recursion_count: see them! */
    
    Scheduler_Mutex_Actions::acquire_action(interpreter, why); 
    sa.be_unable(); 
    mutex->aquire(); 
    if (acquire_ID) trace_mutex_evt(100+acquire_ID, why);
    
    /* if (mutex->check_and_inc_recursion_depth() == 0  &&  
        Scheduler_Mutex_Actions::is_initialized(interpreter)) { 
      Scheduler_Mutex_Actions::acquire_action(interpreter, why); 
      sa.be_unable(); 
      mutex->aquire(); 
      if (acquire_ID) trace_mutex_evt(100+acquire_ID, why);
    } */
    mutex->add_acq_cycles(OS_Interface::get_cycle_count() - start); 
  } 
  
  ~Scheduler_Mutex() { 
    u_int64 start = OS_Interface::get_cycle_count(); 
    if (release_ID) 
      trace_mutex_evt(release_ID, why);
    OS_Mutex_Interface* mutex = 
      Scheduler_Mutex_Actions::get_mutex(interpreter);
    
    /*
    if (mutex->dec_and_check_recursion_depth() <= 0  &&  
        Scheduler_Mutex_Actions::is_initialized(interpreter)) { 
      if (release_ID) trace_mutex_evt(100+release_ID, why);
      * assert_always(mutex->local.recursion_count == 0); * 
      Scheduler_Mutex_Actions::release_action(interpreter, why); 
      mutex->release(); 
    }*/ 
    if (release_ID) trace_mutex_evt(100+release_ID, why);
    /* assert_always(mutex->local.recursion_count == 0); */ 
    mutex->release(); 
    Scheduler_Mutex_Actions::release_action(interpreter, why); 
    
    mutex->add_rel_cycles(OS_Interface::get_cycle_count() - start); 
  } 
};



