//
//  read_barrier.h
//  RoarVM
//
//  Created by Mattias De Wael on 08/08/11.
//  Copyright 2011 VUB. All rights reserved.
//
class Read_Barrier {
public:
  static void doLVB(Oop* p){
    Oop oldValue = *p;
    
    bool NMT_copy = Logical_Core::my_NMT();
    bool NMT_inOop = oldValue.getNMT();
    
    if( NMT_copy != NMT_inOop ){
      on_NMT_trap(p,oldValue);
    }
    /*
     if ( is_pointing_to_protected_page( oldValue ) ) {
     on_Protected_trap(p,oldValue);
     }
     */
  }
  
  static void on_NMT_trap(Oop* p, Oop value);
  
  static void on_Protected_trap(Oop* p, Oop oldValue){
    printf("entered on on_Protected_trap(Oop* p, Oop oldValue)...\n"); return;
    Oop* newAddress = GC_Thread_Class::getInstance()->lookUpNewLocation(oldValue);
    if( newAddress == NULL){ // Not yet moved.
      Object* theObject = oldValue.as_object_in_unprotected_space();
      newAddress = theObject->relocateIfNoForwardingPointer( The_Memory_System()->my_heap() );
      assert(newAddress != NULL);
    }
    Oop::atomic_compare_and_swap(p, oldValue, *newAddress);
  }
};

