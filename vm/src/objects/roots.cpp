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
 *    Reinout Stevens, Vrije Universiteit Brussel - Added scheduler for each core
 ******************************************************************************/


#include "headers.h"

Roots::Roots() {
  FOR_EACH_ROOT(this,oopp) *oopp = uninitialized_value();
}

void Roots::initialize(Oop soo) {
  specialObjectsOop = soo;
    nilObj = The_Squeak_Interpreter()->splObj(Special_Indices::  NilObject);
  falseObj = The_Squeak_Interpreter()->splObj(Special_Indices::FalseObject);
   trueObj = The_Squeak_Interpreter()->splObj(Special_Indices:: TrueObject);
    
    Scheduler* scheduler = new Scheduler();
    scheduler->initialize(The_Squeak_Interpreter());
    The_Squeak_Interpreter()->set_scheduler(scheduler);

  running_process_or_nil = nilObj;
  
# if Extra_Preheader_Word_Experiment
  extra_preheader_word_selector = nilObj;
# endif
  emergency_semaphore = nilObj;
  
  FOR_EACH_READY_PROCESS_LIST(slo, p, processList, The_Squeak_Interpreter()) {
    sched_list_class = processList->fetchClass();
    break;
  }

  flush_freeContexts();
}


void Roots::flush_freeContexts() {
  freeContexts      = Object::NilContext();
  freeLargeContexts = Object::NilContext();
}


bool Roots::verify() {
  FOR_EACH_ROOT(this,oopp) oopp->verify_object();
  return true;
}



