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


template <typename T> class tracked_ptr {
private:
  
  T* _ptr;

public:
  
  tracked_ptr(T* const ptr) : _ptr(ptr) {
    throw 0;
  }
  
  T& operator* () const {
    return *_ptr;
  }
  
  T* operator-> () const {
    return _ptr;
  }
  
  static bool is_registered(T* const ptr) {
    throw 0;
  }
  
  static bool is_valid(T* const ptr) {
    throw 0;
  }
  
  static void invalidate_all_pointer() {
    throw 0;
  }
  
}; // tracked_ptr
