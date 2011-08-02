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
 *    Reinout Stevens, Vrije Universiteit Brussel - Scheduler per interpreter
 ******************************************************************************/


/* the scheduler functionality is a bit scattered over object and interpreter
 * this class tries to bundle that functionality, but does not always succeed 
 * in doing so. 
 * The object-class still modifies the lists of the scheduler,
 * the interpreter has some functionality left too that was not moved here
 * as it modified internal variables of the interpreter */

class Scheduler {
  Oop _schedulerPointer; //a Smalltalk ProcessScheduler object
  Squeak_Interpreter* _interpreter;
  OS_Mutex_Interface scheduler_mutex;
  
public:
  //static bool scheduler_per_interpreter;
  static Squeak_Interpreter* get_interpreter_at_rank(int rank);
  static Squeak_Interpreter* get_random_interpreter();
  Oop schedulerPointer();
  void set_scheduler_pointer(Oop);
  void initialize(Squeak_Interpreter*);
  Object_p schedulerPointer_obj();     
  Object_p process_lists_of_scheduler();
  OS_Mutex_Interface* get_scheduler_mutex();
  void transform_to_scheduler_per_interpreter();
  void transform_to_global_scheduler();
  void* operator new(size_t s) { return Memory_Semantics::shared_malloc(s); }
  Squeak_Interpreter* get_interpreter(){ return _interpreter; };
  Object_p process_list_for_priority(int priority);
  void add_process_to_scheduler_list(Object* process);
  void set_interpreter(Squeak_Interpreter*);
  Oop get_running_process();
  void set_running_process(Oop, const char* why);
  Oop steal_process_from_me(int, int, Squeak_Interpreter*);
  Oop find_and_move_to_end_highest_priority_non_running_process();
  Oop find_non_running_process_for_core_between(int, int, Squeak_Interpreter*);
  int count_processes_in_scheduler();
  void create_new_mutex();
  void set_mutex(OS_Mutex_Interface);
  void transferTo(Oop newProc, const char* why);
  void switch_to_own_mutex();
  void switch_to_shared_mutex();
private:
  
  Oop process_lists_of_scheduler_pointer(int);
  Oop process_lists_of_scheduler_pointer();
  Oop transform_one_interpreter(Object_p linkedListClass);
  OS_Interface::Mutex* shared_mutex;
};

static Squeak_Interpreter* interpreters[Max_Number_Of_Cores] = { NULL };
static bool scheduler_per_interpreter = false;


