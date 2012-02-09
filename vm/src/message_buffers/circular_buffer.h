/******************************************************************************
 *  Copyright (c) 2008 - 2010 IBM Corporation and others.
 *  All rights reserved. This program and the accompanying materials
 *  are made available under the terms of the Eclipse Public License v1.0
 *  which accompanies this distribution, and is available at
 *  http://www.eclipse.org/legal/epl-v10.html
 * 
 *  Contributors:
 *    Stefan Marr, Vrije Universiteit Brussel - Initial Implementation
 ******************************************************************************/


template<typename T>
class Circular_Buffer {
private:
  size_t const size;
  T*     const buffer;
  
  volatile size_t head;
  volatile size_t tail;
  volatile bool   full;
  
public:
  Circular_Buffer(void* const buffer, size_t const num_items)
    : size(num_items), buffer((T* const)buffer), head(0), tail(0), full(false) {}
  
  inline void enqueue(T item) {
    assert(!full);
    
    size_t new_head = (head + 1) % size;
    
    if (new_head == tail)
      full = true;
    
    buffer[new_head] = item;
    head = new_head;
  }
  
  inline T dequeue() {
    if (is_empty())
      return NULL;
    
    tail = (tail + 1) % size;
    full = false;
    return buffer[tail];
  }
  
  inline bool is_empty() { return !full && head == tail; };
};
