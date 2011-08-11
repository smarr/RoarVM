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
#define CONTENTS_SIZE 10000


typedef struct Contents {
  // Moved Contents-struct out of the GC_Oop_Stack class. [MDW]
  int next_elem;
  Object* objs[CONTENTS_SIZE];
  Contents* next_contents;
  Contents(Contents* last) { next_elem = 0; next_contents = last; }
  
  void* operator new(size_t size){
    void *storage = Memory_Semantics::shared_malloc(size);
    assert_always(storage != NULL);
    return storage;
  }
  
  Contents(){ next_contents = NULL; }
  ~Contents() { if (next_contents != NULL) { delete next_contents; next_contents = NULL;  } }
} Contents;

class GC_Oop_Stack {
  
  
public:
  Contents *contents, *free_contents;
  
  static inline bool atomic_compare_and_swap(Contents** ptr, Contents* old_value, Contents* new_value) {
    return __sync_bool_compare_and_swap(ptr, old_value, new_value);
  }
  
  static void tests();
  
  GC_Oop_Stack() {
    //printf("In GC-stack constructor...");
    contents = new Contents(NULL);
    free_contents = NULL;
  }
  
  ~GC_Oop_Stack() {
    if (contents != NULL) { delete contents; contents = NULL;  }
    if (free_contents != NULL) { delete free_contents; free_contents = NULL; }
  }
  
  void verify();
  
  bool isAboutToCreateNewContents();
  
  bool pushWithoutCreatingNewContents(Object* x);
  
  
  Contents* popCurrentContents();
  
  void pushOtherStack( GC_Oop_Stack* otherstack );
  void addNewTopContents(Contents*  newContents );
  
  void push(Object* x);
  
  bool is_empty();
  void setContentsToNextContents();
  Object* pop();
};

