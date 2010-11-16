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
 * This class implements the registry for the tracked pointers.
 * Currently it is a std::set with a simple mutex to guard the set
 * from concurrent modification.
 */
template<typename TrackedPtr>
class tracked_ptr_registry {
private:
  
  typedef set<TrackedPtr*> registry_t;
  
  registry_t registry;
  
  OS_Interface::Mutex lock;
  
public:
  
  typedef typename registry_t::iterator iterator;
  
  
  tracked_ptr_registry() {
    OS_Interface::mutex_init(&lock, NULL);
  }
  
  ~tracked_ptr_registry() {
    OS_Interface::mutex_destruct(&lock);
  }
  
  
  iterator register_tracked_ptr(TrackedPtr* t_ptr) {
    OS_Interface::mutex_lock(&lock);
      pair<typename registry_t::iterator, bool> p = registry.insert(t_ptr);
    OS_Interface::mutex_unlock(&lock);
    
    assert(p.second);
    
    return p.first;
  }
  
  void unregister_tracked_ptr(TrackedPtr* t_ptr, iterator it) {
    if (it != registry.end()) {
      // ensure that the iterator is stable
      assert(*it == t_ptr);
      
      OS_Interface::mutex_lock(&lock);
        registry.erase(it);
      OS_Interface::mutex_unlock(&lock);
    }
  }
  
  void invalidate_all_pointer() {
    OS_Interface::mutex_lock(&lock);
      iterator i;
      for (i = registry.begin(); i != registry.end(); i++) {
        (*i)->valid = false;
      }
    OS_Interface::mutex_unlock(&lock);
  }
  
/*  iterator begin() {
    return registry.begin();
  }

  iterator end() {
    return registry.end();
  }*/
  
  size_t size() {
    return registry.size();
  }
  
}; // tracked_ptr_registry
