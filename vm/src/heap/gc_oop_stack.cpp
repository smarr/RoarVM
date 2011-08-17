//
//  gc_oop_stack.cpp
//  RoarVM
//
//  Created by Mattias De Wael on 08/08/11.
//  Copyright 2011 VUB. All rights reserved.
//

#include "headers.h"

void GC_Oop_Stack::tests(){
  /*
   * This test is NOT complete!
   */
  // Create stack
  GC_Oop_Stack* p_stack = new GC_Oop_Stack();
  
  // Is a newly created stack empty?
  if( !p_stack->is_empty() ) fatal("error in tests\n");
  
  // Can we push objects?
  for(int i=0; i<3*CONTENTS_SIZE ; i++){
    p_stack->push((Object*)(i+1));
  }
  
  // Are the popped objects the objects we pushed?
  int last = (3*CONTENTS_SIZE);
  for(int i=0; i<CONTENTS_SIZE+2 ; i++){
    int t = (int)p_stack->pop();
    if( t != last) fatal("ERROR IN TEST\n");
    last--;
  }
  
  // Is the stack -non-empty if we only popped some elements?
  if( p_stack->is_empty()) fatal("error in tests\n");
  
  for(int i=++last; i<=3*CONTENTS_SIZE ; i++){
    p_stack->push((Object*)i);
  }
  
  // Are all popped objects the objects we pushed?
  last = (3*CONTENTS_SIZE);
  for(int i=0; i<3*CONTENTS_SIZE ; i++){
    int t = (int)p_stack->pop();
    if( t != last) fatal("ERROR IN TEST\n");
    last--;
  }
  
  // After clearing the stack from elements, the Contents shoud be reused.
  if(p_stack->free_contents == NULL)fatal("error in tests\n");
  if(p_stack->free_contents->next_contents == NULL)fatal("error in tests\n");
  if(p_stack->free_contents->next_contents->next_contents != NULL)fatal("error in tests\n");
  
  // Is the empty stack empty?
  if( !p_stack->is_empty()) fatal("error in tests\n");
  
  
  for(int i=++last; i<=3*CONTENTS_SIZE ; i++){
    p_stack->push((Object*)i);
  }
  
  // This the stack has now a full contents block, next call should thus not be allowed. (returns false if fails)
  if(p_stack->pushWithoutCreatingNewContents((Object*)123)) fatal("error in tests\n");
  
  // This stack's top contents has one more spot, next call should succeed?
  p_stack->pop();
  if(!p_stack->pushWithoutCreatingNewContents((Object*)123)) fatal("error in tests\n");
  
  // Now the stack should not be empty.
  if( p_stack->is_empty()) fatal("error in tests\n");
  
  // After popping the current contents the stack should be empty.
  Contents* c = p_stack->popCurrentContents();
  if( !p_stack->is_empty()) fatal("error in tests\n");
  
  delete p_stack;
  printf("Stack test succeeded.\n");
}

bool GC_Oop_Stack::isAboutToCreateNewContents(){
  return (contents->next_elem) >= CONTENTS_SIZE;
}

bool GC_Oop_Stack::pushWithoutCreatingNewContents(Object* x){
  if( isAboutToCreateNewContents() ){
    return false;
  } else {
    contents->objs[(contents->next_elem)++] = x;
    return true;
  }
}


Contents* GC_Oop_Stack::popCurrentContents(){
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

void GC_Oop_Stack::pushOtherStack( GC_Oop_Stack* otherStack ){
  Contents* theContents = NULL;
  Contents* newEmptyContents = new Contents(NULL);
  
  /*
   do{
    theContents = otherStack->contents;
  }  while(atomic_compare_and_swap(&otherStack->contents,theContents,newEmptyContents));
   */
  // if CAS is not needed:
  theContents = otherStack->contents;
  otherStack->contents = newEmptyContents;
  
  Contents* posibleLast = theContents;
  while(posibleLast->next_contents != NULL) posibleLast = posibleLast->next_contents;
  posibleLast->next_contents = this->contents;
  this->contents = theContents;
  
  verify();
}

void GC_Oop_Stack::addNewTopContents(Contents*  newContents ){
  //printf("Stack\taddNewTopContents\n");
  
  assert_always( newContents->next_contents == NULL );
  // if current top-contents is empty we can also replace it...
  newContents->next_contents = contents;
  contents = newContents;
  
  verify();
}

void GC_Oop_Stack::push(Object* x) {
  if( !The_GC_Thread()->is_mark_phase() ) assert( !The_GC_Thread()->isCompletelyDead( x->my_pageNumber() ));
  assert( The_Memory_System()->contains(x) );
  
  assert_always(this != NULL);
  if(x == NULL){
    volatile int a = 1;
  }
  if ( !contents->push(x) ){
    if (free_contents == NULL){
      //printf("Creating new contents block....\n");
      contents = new Contents(contents);
    } else {
      //printf("Using old contents block...\n");
      Contents* c = free_contents;
      free_contents = c->next_contents;
      c->next_contents = contents;
      contents = c;
    }
    push(x);
  }
  assert_always(this != NULL);
  verify();
}

bool GC_Oop_Stack::is_empty() {
  if( contents->is_empty() ){
    if(contents->next_contents != NULL){
      setContentsToNextContents();
      return is_empty();
    } else {
      return true;
    }
  } else {
    return false;
  }
}

void GC_Oop_Stack::setContentsToNextContents(){
  if (contents->next_contents != NULL) {
    Contents* c = contents;
    contents = c->next_contents;
    
    c->next_contents = free_contents;
    free_contents = c;
  }
  verify();
}

Object* GC_Oop_Stack::pop() {
  if(this->is_empty()) fatal("Should not pop when empty.\n");
  /*
   while(contents->is_empty() && contents->hasNextContents()) {
    setContentsToNextContents();
  }
   */
  Object* r = contents->pop();
  verify();
  return r;
}

void GC_Oop_Stack::verify(){
  assert( contents != NULL );
}