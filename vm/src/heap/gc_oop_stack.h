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
    ~Contents() { if (next_contents != NULL) { delete next_contents; next_contents = NULL;  } }
} Contents;

class GC_Oop_Stack {
  

public:
    Contents *contents, *free_contents;
    
    static inline bool atomic_compare_and_swap(Contents** ptr, Contents* old_value, Contents* new_value) {
        return __sync_bool_compare_and_swap(ptr, old_value, new_value);
    }
    
    static void tests(){
      /*
       * This test is NOT complete!
       */
        // Create stack
        GC_Oop_Stack stack;
        
      // Is a newly created stack empty?
        if( !stack.is_empty() ) fatal("error in tests\n");
        
      // Can we push objects?
        for(int i=0; i<3*CONTENTS_SIZE ; i++){
            stack.push((Object*)i);
        }
      
      // Are the popped objects the objects we pushed?
        int last = (3*CONTENTS_SIZE)-1;
        for(int i=0; i<CONTENTS_SIZE+10 ; i++){
            int t = (int)stack.pop();
            if( t != last) fatal("ERROR IN TEST\n");
            last--;
        }
        
      // Is the stack -non-empty if we only popped some elements?
        if( stack.is_empty()) fatal("error in tests\n");
        
        for(int i=++last; i<3*CONTENTS_SIZE ; i++){
            stack.push((Object*)i);
        }
      
      // Are all popped objects the objects we pushed?
        last = (3*CONTENTS_SIZE)-1;
        for(int i=0; i<3*CONTENTS_SIZE ; i++){
            int t = (int)stack.pop();
            if( t != last) fatal("ERROR IN TEST\n");
            last--;
        }
      
      // After clearing the stack from elements, the Contents shoud be reused.
        if(stack.free_contents == NULL)fatal("error in tests\n");
        if(stack.free_contents->next_contents == NULL)fatal("error in tests\n");
        if(stack.free_contents->next_contents->next_contents != NULL)fatal("error in tests\n");
      
      // Is the empty stack empty?
        if( !stack.is_empty()) fatal("error in tests\n");
        
        
        for(int i=++last; i<3*CONTENTS_SIZE ; i++){
            stack.push((Object*)i);
        }
      
      // This the stack has now a full contents block, next call should thus not be allowed. (returns false if fails)
        if(stack.pushWithoutCreatingNewContents((Object*)123)) fatal("error in tests\n");
      
      // This stack's top contents has one more spot, next call should succeed?
        stack.pop();
        if(!stack.pushWithoutCreatingNewContents((Object*)123)) fatal("error in tests\n");
      
      // Now the stack should not be empty.
        if( stack.is_empty()) fatal("error in tests\n");
      
      // After popping the current contents the stack should be empty.
        Contents* c = stack.popCurrentContents();
        if( !stack.is_empty()) fatal("error in tests\n");
        
    }
    
   GC_Oop_Stack() {
       contents = new Contents(NULL);
       free_contents = NULL;
   }
    
    ~GC_Oop_Stack() {
        if (contents != NULL) { delete contents; contents = NULL;  }
        if (free_contents != NULL) { delete free_contents; free_contents = NULL; }
    }
    
    bool isAboutToCreateNewContents(){
        return (contents->next_elem) >= CONTENTS_SIZE;
    }
    
    bool pushWithoutCreatingNewContents(Object* x){
        if( isAboutToCreateNewContents() ){
            return false;
        } else {
            contents->objs[(contents->next_elem)++] = x;
            return true;
        }
    }
   
    
    Contents* popCurrentContents(){
        Contents* returnContents = contents;
        
        if (free_contents == NULL)
            contents = new Contents(NULL);
        else {
            contents = free_contents;
            free_contents = contents->next_contents;
            contents->next_contents = NULL;
        }
        
        
        return  returnContents;
    }
    
  void push(Object* x) {
    if (contents->next_elem < CONTENTS_SIZE)
      contents->objs[contents->next_elem++] = x;
    else {
      if (free_contents == NULL)
        contents = new Contents(contents);
      else {
        Contents* c = free_contents;
        free_contents = c->next_contents;
        c->next_contents = contents;
        contents = c;
      }
      push(x);
    }
  }

  bool is_empty() { return contents->next_elem == 0  &&  contents->next_contents == NULL; }

  Object* pop() {
    Object* r = contents->objs[--(contents->next_elem)];
    if (contents->next_elem == 0) {
      if (contents->next_contents != NULL) {
        Contents* c = contents;
        contents = c->next_contents;

        c->next_contents = free_contents;
        free_contents = c;
      }
    }
    return r;
  }
};

