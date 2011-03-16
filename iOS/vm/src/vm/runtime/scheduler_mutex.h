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
  static void acquire_action(const char*);
  static void release_action(const char*);
  static bool is_initialized();

  static OS_Mutex_Interface* get_mutex();
  static bool is_held();
};

// typedef Abstract_Mutex<Scheduler_Mutex_Actions> Scheduler_Mutex;

Define_RVM_Mutex(Scheduler_Mutex, Scheduler_Mutex_Actions,13,14)

