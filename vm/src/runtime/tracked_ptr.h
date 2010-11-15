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


#include <assert.h>

/**
 * This class is used to track pointers on the stack.
 * The idea is that a global event can occur that invalidates the pointers
 * and this class is meant to signal all cases in which an invalid pointer
 * is used.
 */
template<typename T>
class tracked_ptr {
private:
  
  typedef tracked_ptr_registry< tracked_ptr<T> > registry_t;
  typedef typename tracked_ptr_registry< tracked_ptr<T> >::iterator iterator;
  
  static registry_t registry;
  
  T*       ptr;
  bool     valid;
  iterator it;
  
  iterator register_this() {
    return registry.register_tracked_ptr(this);
  }
    
  void unregister_and_invalidate() {
    registry.unregister_tracked_ptr(this, it);
    valid = false;
  }

public:
  
  /**
   * This operation will invalidate all pointers that are tracked
   * and on subsequent use, an assertion will fail.
   */
  static void invalidate_all_pointer() {
    iterator i;
    for (i = registry.begin(); i != registry.end(); i++) {
      (*i)->valid = false;
    }
  }

  
  
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
  
  inline T& operator* () const {
    assert(is_valid());
    assert(ptr != NULL);
    return *ptr;
  }
  
  inline T* operator-> () const {
    assert(is_valid());
    assert(ptr != NULL);
    return ptr;
  }
  
  inline bool operator==(tracked_ptr const & t_ptr) const {
    return ptr == t_ptr.ptr;
  }

  inline bool operator!=(tracked_ptr const & t_ptr) const {
    return ptr != t_ptr.ptr;
  }
    
  inline T* get() const {
    return ptr;
  }
  
  static size_t pointers_on_stack() {
    return registry.size();
  }  
  
  bool is_valid() const {
    return valid;
  }
    
}; // tracked_ptr

template<typename T>
inline bool operator&&(const bool a, tracked_ptr<T> const & t_ptr) {
  return a && t_ptr.is_valid() && t_ptr.get();
} 

template<typename T>
typename tracked_ptr<T>::registry_t tracked_ptr<T>::registry;
