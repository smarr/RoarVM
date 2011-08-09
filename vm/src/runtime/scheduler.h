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
  Oop _processSchedulerPointer; //a Smalltalk ProcessScheduler object
  Squeak_Interpreter* _interpreter;
  OS_Mutex_Interface scheduler_mutex;
  
public:
  void* operator new(size_t s) { return Memory_Semantics::shared_malloc(s); }
  
  static Squeak_Interpreter* interpreters[Max_Number_Of_Cores];
  static bool scheduler_per_interpreter;
  static Squeak_Interpreter* get_interpreter_at_rank(int rank);
  inline Squeak_Interpreter* get_interpreter() { return _interpreter; }
  void   set_interpreter(Squeak_Interpreter*);

  static Squeak_Interpreter* get_random_interpreter();

  
  void     initialize(Squeak_Interpreter*);
  
  
  Oop      processorSchedulerPointer();
  void     set_scheduler_pointer(Oop);
  
  Object_p processorSchedulerPointer_obj();     
  Object_p process_lists_of_scheduler();
  Object_p process_list_for_priority(int priority);
  void     add_process_to_scheduler_list(Object_p process);
  void     remove_process_from_list(Oop, const char*);
 
  Oop steal_process_from_me_in_range(int hi, int lo, Squeak_Interpreter*);
  Oop find_and_move_to_end_highest_priority_non_running_process();
  Oop find_non_running_process_for_core_between__ACQ(int hi, int lo,Squeak_Interpreter*);
  
  void fixBackPointerOfProcess(Object_p);
  
  
  int count_processes_in_scheduler__ACQ();
  
  void transferTo(Oop newProc, const char* why);
  
  Oop  get_running_process();
  void set_running_process(Oop, const char* why);
  
  
  
  OS_Mutex_Interface* get_scheduler_mutex();
  void create_new_mutex();
  void set_mutex(OS_Mutex_Interface);

  void transform_to_scheduler_per_interpreter();
  void transform_to_global_scheduler();
  void switch_to_own_mutex();
  void switch_to_shared_mutex();
  
private:
  
  Oop process_lists_of_scheduler_pointer(int);
  Oop process_lists_of_scheduler_pointer();
  Oop transform_one_interpreter(Object_p linkedListClass);
  OS_Interface::Mutex* shared_mutex;
  
};

