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
  registry_t* registries[Max_Number_Of_Cores];
  
  int32_t next_rank;
  pthread_key_t registry_key;
  
  inline registry_t* get_registry() {
    registry_t* registry = (registry_t*)pthread_getspecific(registry_key);
    if (registry == NULL) {
      registry = new registry_t();
      pthread_setspecific(registry_key, registry);
      int32_t my_rank = __sync_fetch_and_add(&next_rank, 1);
      registries[my_rank] = registry;
    }
    return registry;
  }
  
public:
  
  typedef typename registry_t::iterator iterator;
  
  
  tracked_ptr_registry() : next_rank(0) {
    for (size_t i = 0; i < Max_Number_Of_Cores; i++) {
      registries[i] = NULL;
    }
    pthread_key_create(&registry_key, NULL);
  }
  
  ~tracked_ptr_registry() {
    pthread_key_delete(registry_key);
  }
  
  iterator register_tracked_ptr(TrackedPtr* t_ptr) {
    registry_t* const registry = get_registry();
    
    pair<typename registry_t::iterator, bool> p = registry->insert(t_ptr);
    
    assert(p.second);
    
    return p.first;
  }
  
  void unregister_tracked_ptr(TrackedPtr* t_ptr, iterator it) {
    registry_t* const registry = get_registry();
    
    if (it != registry->end()) {
      // ensure that the iterator is stable
      assert(*it == t_ptr);
      
      registry->erase(it);
    }
  }
  
  void invalidate_all_pointer() const {
    for (size_t rank = 0; rank < Max_Number_Of_Cores; rank++) {
      registry_t* const registry = registries[rank];
      if (registry) {
        iterator i;
        for (i = registry->begin(); i != registry->end(); i++) {
          (*i)->valid = false;
        }
      }
    }
  }
  
  /* rather useless at the moment */
  size_t size() {
    registry_t* const registry = get_registry();
    return registry->size();
  }
  
}; // tracked_ptr_registry
