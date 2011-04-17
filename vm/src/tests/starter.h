/******************************************************************************
 *  Copyright (c) 2008 - 2010 IBM Corporation and others.
 *  All rights reserved. This program and the accompanying materials
 *  are made available under the terms of the Eclipse Public License v1.0
 *  which accompanies this distribution, and is available at
 *  http://www.eclipse.org/legal/epl-v10.html
 * 
 *  Contributors:
 *    Stefan Marr, Vrije Universiteit Brussel - Initial Implementation
 *
 *  This file encode all necessary helper functionallity to start
 *  threads and make sure they registered on the barriers correctly before
 *  going on in the spawing thread.
 *
 *  This is basically equal to a 'clocked' or 'phased' statement in X10/Habanero. 
 *
 ******************************************************************************/


#ifndef __STARTER_H__
#define __STARTER_H__

#include <pthread.h>
//#include <misc/misc.h>


//#include <stdlib.h>
//#include <stdio.h>
//#include <barriers/barrier.h>

typedef struct starter_t {
  int num_participants;
  volatile bool init_completed;
  pthread_mutex_t global_init_mtx;
  pthread_cond_t  global_init_sig;
} starter_t;

typedef struct thread_param_t {
  starter_t* starter;
  int        id;
  void      (*func)(struct thread_param_t*);
  int        cpu_id;  // only used on tilera at the moment
  pthread_mutex_t lock;
  pthread_cond_t  cond;
  volatile bool initialized;
} thread_param_t;

void thread_param_init(thread_param_t* const param, starter_t* const starter, int id);
void thread_param_ensure_child_initialization(thread_param_t* const tp);
void thread_param_signalAndAwaitInitialization(thread_param_t* const tp);

void starter_init(starter_t* const starter, const int numParticipants);
void starter_await_initalization_to_finish(starter_t* const starter);
void starter_spawn_threads(starter_t* const starter, void (*func)(thread_param_t*), pthread_t* threads);

/**
 * To be called from the managing thread to conclude initialization globally
 */
void starter_signal_initalization_finished(starter_t* const starter);

#endif

