//
//  read_barrier.cpp
//  RoarVM
//
//  Created by Mattias De Wael on 08/08/11.
//  Copyright 2011 VUB. All rights reserved.
//

#include "headers.h"

void Read_Barrier::on_NMT_trap(Oop* p, Oop value);{
  //printf("On NMT trap (%p)(%d)\n",p,value.getNMT());
  Oop nmtCorrectOop = Oop::from_bits(value.bits());
  nmtCorrectOop.setNMT( Logical_Core::my_NMT() );
  // If CAS fails the pointer was already changed: ignore.
  if (Oop::atomic_compare_and_swap(p,value,nmtCorrectOop) ) {
    The_Squeak_Interpreter()->addToBulkOfNMTtrappedRef( nmtCorrectOop );
    //printf("on_NMT_trap changed a pointer\n");
  } else {
    //printf("on_NMT_trap did not change a pointer\n");
  }
  //printf("On NMT trap (handled) (%p)\n",p);
}