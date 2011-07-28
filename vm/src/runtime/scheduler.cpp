/******************************************************************************
 *  Copyright (c) 2008 - 2010 IBM Corporation and others.
 *  All rights reserved. This program and the accompanying materials
 *  are made available under the terms of the Eclipse Public License v1.0
 *  which accompanies this distribution, and is available at
 *  http://www.eclipse.org/legal/epl-v10.html
 * 
 *  Contributors:
 *    Reinout Stevens, Vrije Universiteit Brussel - Initial Implementation
 *    Stefan Marr, Vrije Universiteit Brussel - Initial Implementation
 ******************************************************************************/

#include "headers.h"

bool Scheduler::scheduler_per_interpreter = false;

void Scheduler::initialize(Squeak_Interpreter* interpreter) {
    scheduler_mutex.initialize_globals();
    _interpreter = interpreter;
    _schedulerPointer = 
        _interpreter->splObj(Special_Indices::SchedulerAssociation);
}

/* for reasons unknown the scheduler object is stored in the roots
 * as an association, meaning we get an association object and not a direct
 * access to the scheduler */

Oop Scheduler::schedulerPointer() {
    return _schedulerPointer.as_object()
        ->fetchPointer(Object_Indices::ValueIndex);
}


Object_p Scheduler::schedulerPointer_obj() { 
    return schedulerPointer().as_object(); 
}


Oop Scheduler::process_lists_of_scheduler_pointer(int rank) {
    Oop schedlist = schedulerPointer_obj()
        ->fetchPointer(Object_Indices::ProcessListsIndex);
    Oop plist;
    if(scheduler_per_interpreter) {
        plist = schedlist.as_object()->fetchPointer(rank);
        
    } else {
        plist = schedlist;
    }
    return plist;
}


Oop Scheduler::process_lists_of_scheduler_pointer() {
    return process_lists_of_scheduler_pointer(_interpreter->my_rank());
}


Object_p Scheduler::process_lists_of_scheduler() {
    Object_p plist = process_lists_of_scheduler_pointer().as_object();
    assert(plist->isArray());
    return plist;
}

OS_Mutex_Interface* Scheduler::get_scheduler_mutex() {
    return &scheduler_mutex;
}



/* The older images have 1 global process list which is stored in the roots
 * As this was a bottleneck we provide a scheduler/list for each interpreter
 * Old images will restore the roots with just a list of processes:
 * We convert these here to a list of process lists, for the total of 
 * available interpreters.
 * 
 * This structure cannot be stored in the image either, 
 * as we cannot ensure that the nr of cores
 * remains the same accross startups. 
 * A possible solution is to convert the list of lists to a single
 * list when saving an image (so to the old format), 
 * and rebuild this list here.
 *
 * We hope that we can implement proper workstealing: 
 * this allows us to give the main interpreter all the tasks and redistribute 
 * them in a later stage.
 */

void Scheduler::transform_to_scheduler_per_interpreter(){
    if(scheduler_per_interpreter)
        return;
    int s = _interpreter->makeArrayStart();
    //add process list for main core
    Oop processList = process_lists_of_scheduler_pointer();
    PUSH_FOR_MAKE_ARRAY(processList);
    for(int i = 1; i < Logical_Core::num_cores; ++i){
        //we already created the main one, so we start from 1
        
    }
    Oop array = The_Squeak_Interpreter()->makeArray(s);
    schedulerPointer_obj()
        ->storePointer(Object_Indices::ProcessListsIndex, array);
    scheduler_per_interpreter = true;
}

void Scheduler::transform_to_global_scheduler() {
    if(!scheduler_per_interpreter)
        return;
    /*
    Object_p result = process_lists_of_scheduler();
    Object_p processorArray = schedulerPointer_obj();
    int rank = _interpreter->my_rank();
    Object_p priority_list;
    Object_p processList;
    Object_p addToThisList;
    //for each scheduler
    for(int i = 0; i < processorArray->fetchWordLength(); ++i) {
        if(i == rank)
            continue;
        priority_list = process_lists_of_scheduler_pointer(i).as_object();
        //for each prioritylist on that scheduler
        for(int j = 0; j < priority_list->fetchWordLength(); ++j){
            processList = priority_list->fetchPointer(j).as_object();
            if(processList->isEmptyList())
                continue;
            addToThisList = priority_list->fetchPointer(j).as_object();
            //for each process
            Oop last_proc = 
                processList->fetchPointer(Object_Indices:: LastLinkIndex);
            Oop proc = 
                processList->fetchPointer(Object_Indices::FirstLinkIndex);
            Object_p proc_obj = proc.as_object();
            for(;;) {
                addToThisList->addLastLinkToList(proc);
                if(proc == last_proc)
                    break;
                proc = proc_obj->fetchPointer(Object_Indices::NextLinkIndex);
                proc_obj = proc.as_object();
            }
        }
    }
    processorArray->storePointer(Object_Indices::ProcessListsIndex,
                                 schedulerPointer());
     */
    /* we assume that the conversion is already done at the Smalltalk side */
    if(!scheduler_per_interpreter)
        return;
    Oop result = process_lists_of_scheduler_pointer();
    schedulerPointer_obj()->storePointer(Object_Indices::ProcessListsIndex,
                                         result);
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


// Was: wakeHighestPriority
// xxxxxx factor with remove_process_from_scheduler_list or not -- dmu 4/09
Oop Scheduler::find_and_move_to_end_highest_priority_non_running_process() {
    if (!get_interpreter()->is_ok_to_run_on_me()) {
        return get_interpreter()->roots.nilObj;
    }
    Scheduler_Mutex sm("find_and_move_to_end_highest_priority_non_running_process");
    // return highest pri ready to run
    // see find_a_process_to_run_and_start_running_it
    bool verbose = false;
    bool found_a_proc = false;
    FOR_EACH_READY_PROCESS_LIST(slo, p, processList, _interpreter)  {
        
        if (processList->isEmptyList())
            continue;
        found_a_proc = true;
        if (verbose)
            lprintf("find_and_move_to_end_highest_priority_non_running_process searching list %d\n", p + 1);
        Oop  first_proc = processList->fetchPointer(Object_Indices::FirstLinkIndex);
        Oop   last_proc = processList->fetchPointer(Object_Indices:: LastLinkIndex);
        
        Oop        proc = first_proc;
        Object_p proc_obj = proc.as_object();
        Object_p prior_proc_obj = (Object_p)NULL;
        for (;;)  {
            if (verbose) {
                debug_printer->printf("on %d: find_and_move_to_end_highest_priority_non_running_process proc: ",
                                      get_interpreter()->my_rank());
                proc_obj->print_process_or_nil(debug_printer);
                debug_printer->nl();
            }
            OS_Interface::mem_fence(); // xxxxxx Is this fence needed? -- dmu 4/09
            assert(proc_obj->as_oop() == proc  &&  proc.as_object() == proc_obj);
            if (proc_obj->is_process_running()  ||  !proc_obj->is_process_allowed_to_run_on_this_core())
                ;
            else if (last_proc == proc) {
                return proc;
            }
            else if (first_proc == proc) {
                processList->removeFirstLinkOfList();
                processList->addLastLinkToList(proc);
                return proc;
            }
            else {
                processList->removeMiddleLinkOfList(prior_proc_obj, proc_obj);
                processList->addLastLinkToList(proc);
                return proc;
            }
            if  (last_proc == proc)
                break;
            
            prior_proc_obj = proc_obj;
            proc = proc_obj->fetchPointer(Object_Indices::NextLinkIndex);
            proc_obj = proc.as_object();
        }
    }
    
    // In a 4.0 image running our prims, there is always at least the idle process in the list
    if (get_interpreter()->primitiveThisProcess_was_called()  &&  Object::image_is_pre_4_1()) assert_always(found_a_proc);
    
    OS_Interface::mem_fence(); // xxxxxx Is this fence needed? -- dmu 4/09
    return get_interpreter()->roots.nilObj;
}

int Scheduler::count_processes_in_scheduler(){
    Scheduler_Mutex sm("count_processes_in_scheduler");
    // return highest pri ready to run
    // see find_a_process_to_run_and_start_running_it
    int count = 0;
    
    FOR_EACH_READY_PROCESS_LIST(slo, p, processList, _interpreter)  {
        
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
    
    Scheduler_Mutex sm("transferTo"); // in case another cpu starts running this
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
