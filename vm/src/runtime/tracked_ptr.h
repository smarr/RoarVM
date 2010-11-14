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


#include <set>
#include <assert.h>

using namespace std;

template<typename T>
class tracked_ptr {
private:
  
  typedef set<tracked_ptr<T>*> registry_t;
  
  static registry_t registry;
  
  T* const _ptr;
  bool valid;
  typename registry_t::iterator it;

public:
  
  tracked_ptr(T* const ptr) :
    _ptr(ptr),
    valid(true)/*,
    it(registry.insert(this))*/ {
      pair<typename registry_t::iterator, bool> p = registry.insert(this);
      assert(p.second);
      it = p.first;
    }
  
  ~tracked_ptr() {
    // ensure that the iterator is stable
    assert(*it == this);
    registry.erase(it);
  }
  
  T& operator* () const {
    assert(is_valid());
    return *_ptr;
  }
  
  T* operator-> () const {
    assert(is_valid());
    return _ptr;
  }
  
  static size_t pointers_on_stack() {
    return registry.size();
  }  
  
  bool is_valid() const {
    return valid;
  }
  
  static void invalidate_all_pointer() {
    typename registry_t::iterator i;
    for (i = registry.begin(); i != registry.end(); i++) {
      (*i)->valid = false;
    }
  }
  
}; // tracked_ptr


template<typename T>
set<tracked_ptr<T>*> tracked_ptr<T>::registry;
