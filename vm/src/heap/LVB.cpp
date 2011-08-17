#include "headers.h"

bool on_NMT_trap(Oop* p, Oop value){
  Oop nmtCorrectOop = Oop::from_bits(value.bits());
  nmtCorrectOop.setNMT( Logical_Core::my_NMT() );
  
  // If CAS fails the pointer was already changed: ignore.
  bool CAS_success = Oop::atomic_compare_and_swap(p,value,nmtCorrectOop);
  
  if( CAS_success && !Logical_Core::running_on_GC() ){
    The_Squeak_Interpreter()->recordNMTtrappedOop( value );
  }
  
  assert(p->getNMT() == Logical_Core::my_NMT());
  assert(p->raw_bits() != value.raw_bits());
  
  return CAS_success;
}

void on_Protected_trap(Oop* p, Oop oldValue){
  //printf("entered on on_Protected_trap(Oop* p, Oop oldValue)...\n");
  Oop newAddress = The_GC_Thread()->lookUpNewLocation(oldValue);
  if( newAddress.raw_bits() == 0){ // Not yet moved.
    //printf("relocateIfNoForwardingPointer\n");
    if ( !The_GC_Thread()->is_relocate_phase() ){
      printf("PAGE %d\n",oldValue.as_object_noLVB()->my_pageNumber());
    }
    assert( The_GC_Thread()->is_relocate_phase() );
    Object* theObject = oldValue.as_object_in_unprotected_space();
    newAddress = theObject->relocateIfNoForwardingPointer( The_Memory_System()->my_heap() );
    assert(newAddress.as_object_noLVB() != NULL);
    
    

  } else {
    /**/
  }
  if(Oop::atomic_compare_and_swap(p, oldValue, newAddress)){
    /**/
  }
  
  
  if(The_GC_Thread()->is_remap_phase()){
    //printf("new addr was %p.\n", newAddress.as_object_noLVB());
  }
  
  
 }

bool is_pointing_to_protected_page_slowVersion(Oop oop){
  Object* obj = (Object*)oop.bits();
  //assert( !The_GC_Thread()->isCompletelyDead( obj->my_pageNumber() ));
  return   The_GC_Thread()->isAlmostDead( obj->my_pageNumber() )      // during relocate phase
        || The_GC_Thread()->isCompletelyDead( obj->my_pageNumber() ); // after relocation (liveness array is modified !!!)
}

bool is_pointing_to_protected_page(Oop oop){
  
  return is_pointing_to_protected_page_slowVersion(oop);
  
  //printf("Entered is_pointing_to_protected_page(Oop oop)\n");
  int r_eax = -1;
  int32 x = oop.bits(); // Get bits() instead of as_object to avoid trapping in LVB again.
  
  Memory_System* mem = The_Memory_System();
  
  /*
   * Block of asembly:
   * - Move zero to eax-register.
   * - Dead-load the object pointed to by 'oop'.
   *  -> this will store TRAPPED in eax if the load was on protected page.
   * -Move the content of eax-register to the r_eax varible.
   * 
   * Verification of the value of r_eax now allows us to determine 
   * if we have been 'trapped' or not.
   */
  asm volatile ("movl $0 , %%eax\n" :"=r"(r_eax));
  asm volatile ("movl (%0), %%ebx\n" :"=r"(x)); // deref!
  asm volatile ("movl %%eax, %0\n" :"=r"(r_eax));
  
  bool res = (r_eax == TRAPPED);
  if(res)printf("Returning from is_pointing_to_protected_page (%s,%d)\n",res?"true":"false",r_eax);
  return res;
}

void doLVB(Oop* p){
  Oop oldValue = Oop::from_bits(p->raw_bits());
  
  
  if(  The_GC_Thread()->is_mark_phase()  && (oldValue.getNMT() != Logical_Core::my_NMT()) && (The_GC_Thread()->isAllocated(oldValue.as_object_noLVB()->my_pageNumber())) ){
    on_NMT_trap(p,oldValue);
    
  } else {
    /* DO NOTHING - all is ok*/
  }
  if ( ( The_GC_Thread()->is_relocate_phase() || The_GC_Thread()->is_remap_phase() ) && is_pointing_to_protected_page( oldValue ) ) {
    on_Protected_trap(p,oldValue);
  }
}
