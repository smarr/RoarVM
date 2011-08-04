/******************************************************************************
 *  Copyright (c) 2008 - 2011 IBM Corporation and others.
 *  All rights reserved. This program and the accompanying materials
 *  are made available under the terms of the Eclipse Public License v1.0
 *  which accompanies this distribution, and is available at
 *  http://www.eclipse.org/legal/epl-v10.html
 * 
 *  Contributors:
 *    Stefan Marr, Vrije Universiteit Brussel - Initial Implementation
 ******************************************************************************/


# if !Use_Object_Table

/**
 * The dummy object table is currently used to assist with reading images,
 * since it shares the necessary functionallity for that with the real
 * object table implementation.
 *
 * Other then that, it just provides the interface and returns the
 * expected values to satisfy assertions.
 */
class Dummy_Object_Table : public Segmented_Object_Table {
public:

  /** Designed to satisfy assertions in the absence of an OT */
  inline bool verify_after_mark()           const { return true; }
  inline bool is_OTE_free(Oop)              const { return true; }
  inline bool probably_contains(void*)      const { return true; }
  inline bool probably_contains_not(void*)  const { return true; }
  
  /** Empty dummies */
  inline void pre_store_whole_enchillada()  const {}
  inline void post_store_whole_enchillada() const {}
  inline void free_oop(Oop COMMA_DCL_ESB)   const {}

  Oop  get_stats(int /* rank */);

  inline Oop allocate_oop_and_set_preheader(Object_p obj, int /* r */ COMMA_DCL_ESB) {
    obj->init_extra_preheader_word();
  }
};

# endif // if !Use_Object_Table
