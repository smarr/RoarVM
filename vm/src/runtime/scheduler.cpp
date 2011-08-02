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

#include "headers.h"


/* Scheduler Structure
 * ===================
 *
 * The older images have 1 global process list which is stored in the roots.
 * As this was a bottleneck we provide a scheduler/list for each interpreter.
 *
 * Old images will restore the roots with just a list of processes:
 * We convert these here to a list of process lists, for the total of 
 * available interpreters.
 * 
 * This structure cannot be stored in the image either, 
 * as we cannot ensure that the number of cores
 * remains the same across startups. 
 * Our solution is to convert the list of lists to a single
 * list when saving an image (so to the old format).
 * 
 * We convert the process list on startup/shutdown time of the image in
 * SystemDictionary snapshot:andQuit:
 * This operation is not safe for multiple interpreters, hence we use the
 * functionality to suspend interpreters, ensuring we are running single
 * threaded. (See RVMOperations>>#suspendAllButMainInterpreter)
 *
 * Scheduling Strategy
 * ===================
 *
 * We have very naive workstealing, in which a scheduler asks a random scheduler
 * for work in case he himself has none. This strategy is subjected to change
 * depending on the outcome of some of the benchmarks */


bool Scheduler::scheduler_per_interpreter = false;
Squeak_Interpreter* Scheduler::interpreters[Max_Number_Of_Cores] = { NULL };

void Scheduler::initialize(Squeak_Interpreter* interpreter) {
  scheduler_mutex.initialize_globals();
  shared_mutex = scheduler_mutex.get_inner_mutex();
  
  set_interpreter(interpreter);
  _processSchedulerPointer = interpreter->splObj(Special_Indices::SchedulerAssociation);
}

/* for reasons unknown the Smalltalk ProcessScheduler object is stored in the
 * roots as an association, meaning we get an association object and not a
 * direct access to the scheduler */
Oop Scheduler::processorSchedulerPointer() {
  return _processSchedulerPointer.as_object()->fetchPointer(Object_Indices::ValueIndex);
}


Object_p Scheduler::processorSchedulerPointer_obj() { 
  return processorSchedulerPointer().as_object(); 
}


Oop Scheduler::process_lists_of_scheduler_pointer(int rank) {
  Oop schedList = processorSchedulerPointer_obj()
                            ->fetchPointer(Object_Indices::ProcessListsIndex);
  
  Oop plist;
  if (scheduler_per_interpreter) {
    plist = schedList.as_object()->fetchPointer(rank);
    
  } else {
    plist = schedList;
  }
  return plist;
}


Oop Scheduler::process_lists_of_scheduler_pointer() {
  return process_lists_of_scheduler_pointer(get_interpreter()->my_rank());
}


Object_p Scheduler::process_lists_of_scheduler() {
  Object_p plist = process_lists_of_scheduler_pointer().as_object();
  assert(plist->isArray());
  return plist;
}

OS_Mutex_Interface* Scheduler::get_scheduler_mutex() {
  return &scheduler_mutex;
}

void Scheduler::set_interpreter(Squeak_Interpreter* interpreter) {
  assert(interpreter->my_rank() < Max_Number_Of_Cores);
  assert(interpreter->my_rank() >= 0);
  interpreters[interpreter->my_rank()] = interpreter;
  _interpreter = interpreter;
}


void Scheduler::switch_to_own_mutex() {
  //we always create a new mutex
  OS_Interface::Mutex* mtx = scheduler_mutex.get_inner_mutex();
  if (shared_mutex == mtx) {
    //ensuring we are not running on our own mutex
    OS_Interface::Mutex* newMutex =
        (OS_Interface::Mutex*)Memory_Semantics::shared_malloc(sizeof(OS_Interface::Mutex));
    OS_Interface::mutex_init(newMutex);
    
    scheduler_mutex.set_inner_mutex(newMutex);
  }
}

void Scheduler::switch_to_shared_mutex() {
  OS_Interface::Mutex* mtx = scheduler_mutex.get_inner_mutex();
  if (shared_mutex != mtx) {
    //ensuring we are not running on the shared mutex
    //freeing own mutex
    free(scheduler_mutex.get_inner_mutex());
    scheduler_mutex.set_inner_mutex(shared_mutex);
  }
}


Oop Scheduler::transform_one_interpreter(Object_p linkedListClass) {
  int s = get_interpreter()->makeArrayStart();
  Object_p linkedList;
  
  FOR_EACH_READY_PROCESS_LIST(slo, p, obj, get_interpreter()) {
    linkedList = linkedListClass->instantiateClass(0);
    PUSH_FOR_MAKE_ARRAY(linkedList->as_oop());
  }
  
  return get_interpreter()->makeArray(s);
}


void Scheduler::transform_to_scheduler_per_interpreter() {
  if (scheduler_per_interpreter)
    return;
  
  int s = get_interpreter()->makeArrayStart();
  Oop processList = process_lists_of_scheduler_pointer();
  
  Object_p listOfPriorityOne = processList.as_object()->fetchPointer(0).as_object();
  Object_p linkedListClass   = listOfPriorityOne->fetchClass().as_object();
  Object_p linkedList;
  
  // everything before the main core
  for (int i = 0; i < Logical_Core::main_rank; ++i) {
    PUSH_FOR_MAKE_ARRAY(transform_one_interpreter(linkedListClass));
  }
  
  // add process list for main core
  PUSH_FOR_MAKE_ARRAY(processList);
  
  // we already created the main one, so we start from main_rank + 1
  for (int i = Logical_Core::main_rank + 1; i < Logical_Core::num_cores; ++i) {
    PUSH_FOR_MAKE_ARRAY(transform_one_interpreter(linkedListClass));
  }
  
  Oop arrayOfArrayOfPriorityLists = The_Squeak_Interpreter()->makeArray(s);
  processorSchedulerPointer_obj()->storePointer(Object_Indices::ProcessListsIndex,
                                       arrayOfArrayOfPriorityLists);
  scheduler_per_interpreter = true;
}


void Scheduler::transform_to_global_scheduler() {
  if (!scheduler_per_interpreter)
    return;
  
  // we reuse the currect process_list
  Object_p result = process_lists_of_scheduler();
  
  // the complete datastructure
  Object_p processorArray = processorSchedulerPointer_obj()
                ->fetchPointer(Object_Indices::ProcessListsIndex).as_object();
  
  int my_rank = get_interpreter()->my_rank();
  Object_p schedArray;
  Object_p processList;
  Object_p addToThisList;
  
  //for each scheduler
  for (int schedId = 0; schedId < processorArray->fetchWordLength(); ++schedId) {
    if (schedId == my_rank) // no need to add our own processes
      continue;
    
    schedArray = process_lists_of_scheduler_pointer(schedId).as_object();
    assert(schedArray->isArray());
    // for each priority list on that scheduler
    for (int priority = 0; priority < schedArray->fetchWordLength(); ++priority) {
      processList = schedArray->fetchPointer(priority).as_object();
      if(processList->isEmptyList())
        continue;
      
      addToThisList = result->fetchPointer(priority).as_object();
      
      // for each process
      Oop last_proc = 
        processList->fetchPointer(Object_Indices:: LastLinkIndex);
      Oop proc = 
        processList->fetchPointer(Object_Indices::FirstLinkIndex);
      Object_p proc_obj = proc.as_object();
      
      for (;;) {
        addToThisList->addLastLinkToList(proc);
        if (proc == last_proc)
          break;
        proc = proc_obj->fetchPointer(Object_Indices::NextLinkIndex);
        proc_obj = proc.as_object();
      }
    }
  }
  processorSchedulerPointer_obj()->storePointer(Object_Indices::ProcessListsIndex,
                                                result->as_oop());
  scheduler_per_interpreter = false;
}

Object_p Scheduler::process_list_for_priority(int priority) {
  Object_p processLists = process_lists_of_scheduler();
  assert(priority - 1 <= processLists->fetchWordLength());
  
  return processLists->fetchPointer(priority - 1).as_object();
}

void Scheduler::add_process_to_scheduler_list(Object* process){
  process_list_for_priority(process->priority_of_process())
                                      ->addLastLinkToList(process->as_oop());
}

Oop Scheduler::get_running_process(){
  return get_interpreter()->get_running_process();
}

void Scheduler::set_running_process(Oop proc, const char* why){
  get_interpreter()->set_running_process(proc, why);
}



Oop Scheduler::steal_process_from_me(int hi, int lo, Squeak_Interpreter* thief) {
  Oop process = find_non_running_process_for_core_between(hi, lo, thief);
  if (process != get_interpreter()->roots.nilObj){
    // set correct backpointer for process
    int priority = process.as_object()->priority_of_process();
    Object_p correctList = thief->get_scheduler()->process_list_for_priority(priority);
    process.as_object()->storePointer(Object_Indices::MyListIndex,
                                      correctList->as_oop());
  }
  return process;
}



// Was: wakeHighestPriority
// xxxxxx factor with remove_process_from_scheduler_list or not -- dmu 4/09
Oop Scheduler::find_and_move_to_end_highest_priority_non_running_process() {
  if (!get_interpreter()->is_ok_to_run_on_me()) {
    return get_interpreter()->roots.nilObj;
  }
  int nrSlices = 4;
  int length = process_lists_of_scheduler()->fetchWordLength();
  if(Logical_Core::num_cores == 1 && !scheduler_per_interpreter) {
    return find_non_running_process_for_core_between(length, 0, get_interpreter());
  }
  int sliceSize = length / nrSlices;
  assert(sliceSize * nrSlices == length);
  for(int i = nrSlices; i > 0; --i) {
    int hi = i * sliceSize;
    int lo = hi - sliceSize;
    Oop result = 
      find_non_running_process_for_core_between(hi, lo, get_interpreter());

	if(result != get_interpreter()->roots.nilObj)
      return result;

    //now we try to steal a process, we are such scallywags
    int my_rank = get_interpreter()->my_rank();
    while(my_rank != get_interpreter()->my_rank()){
      my_rank = rand() % Logical_Core::num_cores; // STEFAN: rand() might be a bottleneck
    }

    Squeak_Interpreter* owner = interpreters[my_rank];

    Oop stolen = owner->scheduler.steal_process_from_me(hi, lo, get_interpreter());
    if (stolen != get_interpreter()->roots.nilObj) {
      printf("STOLEN\n");
      return stolen;
    }
  }
  return get_interpreter()->roots.nilObj;
}


Oop Scheduler::find_non_running_process_for_core_between(int hi, int lo, Squeak_Interpreter* runner) {
  Scheduler_Mutex sm("find_and_move_to_end_highest_priority_non_running_process",
                     get_interpreter());
  // return highest pri ready to run
  // see find_a_process_to_run_and_start_running_it
  bool verbose = false;
  bool found_a_proc = false;
  
  FOR_EACH_READY_PROCESS_LIST(slo, p, processList, get_interpreter())  {  
    if (processList->isEmptyList())
      continue;
    
    found_a_proc = true;
    if (verbose)
      lprintf("find_and_move_to_end_highest_priority_non_running_process searching list %d\n", p + 1);
    Oop  first_proc = processList->fetchPointer(Object_Indices::FirstLinkIndex);
    Oop   last_proc = processList->fetchPointer(Object_Indices:: LastLinkIndex);
    
    Oop      proc_oop = first_proc;
    Object_p proc_obj = proc_oop.as_object();
    Object_p prior_proc_obj = (Object_p)NULL;
    
    for (;;) {
      assert(proc_obj->fetchClass().as_object()->className().as_object()
             ->equals_string("Process"));
      if (verbose) {
        debug_printer->printf("on %d: find_and_move_to_end_highest_priority_non_running_process proc: ",
                              get_interpreter()->my_rank());
        proc_obj->print_process_or_nil(debug_printer);
        debug_printer->nl();
      }
      OS_Interface::mem_fence(); // xxxxxx Is this fence needed? -- dmu 4/09
      assert(proc_obj->as_oop() == proc_oop  &&  proc_oop.as_object() == proc_obj);
      if (proc_obj->is_process_running()  ||  !proc_obj->is_process_allowed_to_run_on_given_interpreter_instance(runner))
        ;
      else if (last_proc == proc_oop) {
        return proc_oop;
      }
      else if (first_proc == proc_oop) {
        processList->removeFirstLinkOfList();
        processList->addLastLinkToList(proc_oop);
        return proc_oop;
      }
      else {
        processList->removeMiddleLinkOfList(prior_proc_obj, proc_obj);
        processList->addLastLinkToList(proc_oop);
        return proc_oop;
      }
      if  (last_proc == proc_oop)
        break;
      
      prior_proc_obj = proc_obj;
      proc_oop = proc_obj->fetchPointer(Object_Indices::NextLinkIndex);
      proc_obj = proc_oop.as_object();
    }
  }
  return get_interpreter()->roots.nilObj;
}



int Scheduler::count_processes_in_scheduler() {
  Scheduler_Mutex sm("count_processes_in_scheduler");
  // return highest pri ready to run
  // see find_a_process_to_run_and_start_running_it
  int count = 0;
  
  FOR_EACH_READY_PROCESS_LIST(slo, p, processList, get_interpreter())  {
    
    if (processList->isEmptyList())
      continue;
    Oop  first_proc = processList->fetchPointer(Object_Indices::FirstLinkIndex);
    Oop   last_proc = processList->fetchPointer(Object_Indices:: LastLinkIndex);
    
    Oop        proc = first_proc;
    Object_p proc_obj = proc.as_object();
    for (;;)  {
      ++count;
      if  (last_proc == proc)
        break;
      
      proc = proc_obj->fetchPointer(Object_Indices::NextLinkIndex);
      proc_obj = proc.as_object();
    }
  }
  
  return count;
}


void Scheduler::transferTo(Oop newProc, const char* why){
  if (check_many_assertions) assert(!newProc.as_object()->is_process_running());
  
  Scheduler_Mutex sm("transferTo"); // in case another core starts running this
  if (Print_Scheduler_Verbose) {
    debug_printer->printf("scheduler: on %d: ", get_interpreter()->my_rank());
    get_running_process().print_process_or_nil(debug_printer);
    debug_printer->printf( " transferTo ");
    newProc.print_process_or_nil(debug_printer);
    debug_printer->printf(", %s\n", why);
  }
  if (check_many_assertions) assert(!newProc.as_object()->is_process_running());
  get_interpreter()->put_running_process_to_sleep(why);
  if (check_many_assertions) assert(!newProc.as_object()->is_process_running());
  OS_Interface::mem_fence();
  get_interpreter()->start_running(newProc, why);
}


Squeak_Interpreter* Scheduler::get_interpreter_at_rank(int rank) {
  assert(rank >= 0);
  assert(rank < Max_Number_Of_Cores);
  return interpreters[rank];
}


Squeak_Interpreter* Scheduler::get_random_interpreter() {
  int r = random() % Logical_Core::num_cores;
  return get_interpreter_at_rank(r);
}