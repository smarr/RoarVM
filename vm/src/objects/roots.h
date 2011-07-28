/******************************************************************************
 *  Copyright (c) 2008 - 2010 IBM Corporation and others.
 *  All rights reserved. This program and the accompanying materials
 *  are made available under the terms of the Eclipse Public License v1.0
 *  which accompanies this distribution, and is available at
 *  http://www.eclipse.org/legal/epl-v10.html
 * 
 *  Contributors:
 *    David Ungar, IBM Research - Initial Implementation
 *    Sam Adams, IBM Research - Initial Implementation
 *    Stefan Marr, Vrije Universiteit Brussel - Port to x86 Multi-Core Systems
 ******************************************************************************/


class Roots {
  Oop uninitialized_value() { return Oop::from_int(0); }
 public:
  Roots();
  void initialize(Oop);
  bool is_initialized() { return nilObj != uninitialized_value(); }

  Oop specialObjectsOop;
  Oop nilObj, falseObj, trueObj;
  Oop freeContexts, freeLargeContexts;

  Oop receiverClass;
  Oop newNativeMethod;
  Oop methodClass; // unused?
  Oop sched_list_class;
  Oop dnuSelector; // for debugging

  // these get sent for control transfers
  Oop _activeContext;
  Oop _method;
  Oop _theHomeContext;
  Oop receiver;
  Oop messageSelector;
  Oop newMethod;
  Oop lkupClass;
  Oop running_process_or_nil;
  
# if Extra_Preheader_Word_Experiment
  Oop extra_preheader_word_selector;
# endif
  Oop emergency_semaphore;


  void flush_freeContexts();
    void transform_process_list();

  bool verify();

};

# define FOR_EACH_ROOT(roots_ptr,oop_ptr) \
  for ( Oop* oop_ptr = (Oop*) (roots_ptr); \
             oop_ptr < (Oop*)((roots_ptr) + 1); \
           ++oop_ptr )

