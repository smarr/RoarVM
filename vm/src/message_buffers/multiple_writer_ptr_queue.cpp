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
 ******************************************************************************/


#include "headers.h"

#include <queue>

MultipleWriterPtrQueue::MultipleWriterPtrQueue() {
  queue = new std::queue<const void*>();

  pthread_mutex_init(&lock, NULL);
}

void MultipleWriterPtrQueue::enqueue(const void* element) {
  pthread_mutex_lock(&lock);
    queue->push(element);
  pthread_mutex_unlock(&lock);
}

const void* MultipleWriterPtrQueue::dequeue() {
  const void* result;

  pthread_mutex_lock(&lock);
    result = queue->front();
    queue->pop();
  pthread_mutex_unlock(&lock);

  return result;
}

bool MultipleWriterPtrQueue::empty() {
  return queue->empty();
}

