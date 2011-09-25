/******************************************************************************
 *  Copyright (c) 2008 - 2011 IBM Corporation and others.
 *  All rights reserved. This program and the accompanying materials
 *  are made available under the terms of the Eclipse Public License v1.0
 *  which accompanies this distribution, and is available at
 *  http://www.eclipse.org/legal/epl-v10.html
 * 
 *  Contributors:
 *    Mattias De Wael, Vrije Universiteit Brussel - Parallel Garbage Collection
 *    Wouter Amerijckx, Vrije Universiteit Brussel - Parallel Garbage Collection
 ******************************************************************************/


class SetNMTbitAndCollectRoots_Oop_Closure: public Oop_Closure {
public:
  SetNMTbitAndCollectRoots_Oop_Closure(int NMT) : Oop_Closure() {
    roots = new GC_Oop_Stack();
    m_NMT = NMT;
  }
  
  int m_NMT;
  GC_Oop_Stack* roots;
  
  void value(Oop* p, Object_p) {
    bool succeeded = false;
    if (!p->is_mem()) return;
    do{
      Oop newOop = Oop::from_bits(p->bits());
      newOop.setNMT(m_NMT);
      succeeded = Oop::atomic_compare_and_swap(p,*p,newOop); // Actually this should be already thread-safe (overkill?)
    } while(!succeeded);
    Object* theObject = p->as_object();
    roots->push( theObject );
  }
  
  virtual const char* class_name(char*) { return "SetNMTbitAndCollectRoots_Oop_Closure"; }
  
};


class ScrubExistingStaleRefs_Oop_Closure: public Oop_Closure {
public:
  ScrubExistingStaleRefs_Oop_Closure() : Oop_Closure() {}
  
  void value(Oop* p, Object_p) { 
    //Scrub by dereferencing.
    if(p->is_mem()) {
      p->as_object();
      assert(p->raw_bits() != 2);
    }
    
    else { /* do integers need to be moved while scrubbing */ }
  }
  
  virtual const char* class_name(char*) { return "ScrubExistingStaleRefs_Oop_Closure"; }
};


class CollectRoots_Oop_Closure: public Oop_Closure {
public:
  CollectRoots_Oop_Closure() : Oop_Closure() {
    roots = new GC_Oop_Stack();
  }
  
  int m_NMT;
  GC_Oop_Stack* roots;
  

  void value(Oop* p, Object_p) {
    bool succeeded = false;
    if (!p->is_mem()) return;
    
    Object* o = p->as_object();
    roots->push( o );
  }
  
  virtual const char* class_name(char*) { return "CollectRoots_Oop_Closure"; }
  
};
