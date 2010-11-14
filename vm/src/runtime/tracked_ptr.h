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

/**
 * This class is used to track pointers on the stack.
 * The idea is that a global event can occur that invalidates the pointers
 * and this class is meant to signal all cases in which an invalid pointer
 * is used.
 *
 * WARNING: This class is not yet thread-safe in any way.
 */
template<typename T>
class tracked_ptr {
private:
  
  typedef set<tracked_ptr<T>*>          registry_t;
  typedef typename registry_t::iterator iterator;
  
  static registry_t registry;
  
  T*       ptr;
  bool     valid;
  iterator it;
  
  iterator register_this() {
    size_t numElements = registry.size();
    pair<typename registry_t::iterator, bool> p = registry.insert(this);
    size_t numElements2 = registry.size();
    assert(p.second);
    assert(numElements + 1 == numElements2);
    return p.first;
  }
    
  void unregister_and_invalidate() {
    if (it != registry.end()) {
      // ensure that the iterator is stable
      assert(*it == this);
      registry.erase(it);
    }
    valid = false;
  }

public:
  
  /**
   * The simple constructor will initialize the tracker to be invalid.
   * It will also register it, then we don't need to care about to many
   * different cases, I think.
   * The only thing we have to keep track is the pointer passed trough
   * in copy operations, and its validity.
   */
  tracked_ptr() :
    ptr(NULL), valid(false), it(register_this()) {}
  
  /**
   * Constructor initialized with the pointer to be tracked.
   */
  tracked_ptr(T* const ptr) :
    ptr(ptr), valid(true), it(register_this()) {}
  
  /**
   * Copy constructor
   */
  tracked_ptr(const tracked_ptr & t_ptr) :
    ptr(t_ptr.ptr), valid(t_ptr.valid), it(register_this()) {}
  
  ~tracked_ptr() {
    unregister_and_invalidate();
  }  
  
  tracked_ptr& operator=(tracked_ptr const & t_ptr) {
    ptr   = t_ptr.ptr;
    valid = t_ptr.valid;
    
    return *this;
  }
  
  tracked_ptr& operator=(tracked_ptr & t_ptr) {
    ptr   = t_ptr.ptr;
    valid = t_ptr.valid;
    return *this;
  }
  
  T& operator* () const {
    assert(is_valid());
    return *ptr;
  }
  
  T* operator-> () const {
    assert(is_valid());
    return ptr;
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
typename tracked_ptr<T>::registry_t tracked_ptr<T>::registry;
