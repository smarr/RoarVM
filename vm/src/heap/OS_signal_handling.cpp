#include "headers.h"

void signal_handler_setEax(int sig, siginfo_t *info, ucontext_t *uap){
  printf("In signal handler...\n");
  //uap->uc_mcontext->__ss.__eip+=6; // write
  uap->uc_mcontext->__ss.__eip+=7; //read
  uap->uc_mcontext->__ss.__eax=TRAPPED;
}

int install_signalhandler(int signum,  void* sig_handler){
  struct sigaction sa;
  
  sa.sa_handler = (void(*)(int))sig_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_SIGINFO | SA_NODEFER;    
  
  return sigaction(signum, &sa, NULL);
}

int install_signalhandler_protectedPageAcces(){
  install_signalhandler(SIGSEGV, (void*)signal_handler_setEax);
  install_signalhandler(SIGBUS, (void*)signal_handler_setEax);
}

void TEST_force_protectedPage_signal_trap(void* p){
  bool doProtection = true;
  
  Object* o = (Object*)p;
  
  Chunk* c = o->my_chunk();
  int pageSize = getpagesize();
  
  printf("Pointer void:\t%p\n",p);
  printf("Pointer object:\t%p\n",o);
  printf("Pointer chunk:\t%p\n",o);
  
  int oModPS =((int)o%pageSize);
  int oPageAligned = (int)o - oModPS;
  
  install_signalhandler_protectedPageAcces();
  if( doProtection ){
    int prot_res = mprotect((void*)oPageAligned,pageSize,PROT_NONE);
    if(prot_res < 0){
      if( errno == EACCES) printf("EACCES\n");
      if( errno == EINVAL) printf("EINVAL\n");
      if( errno == ENOTSUP) printf("ENOTSUP\n");
    } else {
      printf("Protection in object ok\n");
    }
  }
  
  // Do read barrier...
  The_Squeak_Interpreter()->doLVB(o->as_oop_p());
  
  if( doProtection ){
    mprotect((void*)oPageAligned,pageSize,PROT_READ | PROT_WRITE);
  }
}

void TEST_force_protectedPage_signal_trap(){
  return;
  install_signalhandler_protectedPageAcces();
  Object* v_ptr = (Object*)mmap(NULL,20*sizeof(int),PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON , NULL, 0); //OK
  //int* v_ptr = (int*)malloc(20*sizeof(int)); //NOT ok: not aligned?
  //int* v_ptr = (int*)valloc(20*sizeof(int)); //OK
  
  if(v_ptr == MAP_FAILED){
    printf("MMAP failed(%s)\n",strerror(errno));
    if( errno == EINVAL) printf("EINVAL\n");
  } else {
    printf("Pointer to int:%p\n",v_ptr);
    printf("page size = %d\n",getpagesize());
    
    int prot_res = 1;//mprotect(v_ptr,10*sizeof(int),PROT_NONE);
    if(prot_res < 0){
      if( errno == EACCES) printf("EACCES\n");
      if( errno == EINVAL) printf("EINVAL\n");
      if( errno == ENOTSUP) printf("ENOTSUP\n");
    } else {
    
    printf("Protection result:%d\n",prot_res);
    
    int r_eax;
    *((int*)v_ptr) = 4; // write
    asm volatile ("movl %%eax, %0\n" :"=r"(r_eax));
    if(r_eax == TRAPPED){
      printf("TRAPPED, returning 1\n");
      printf("TRAPPED, returning 2\n");
    } else {
      printf("NOT TRAPPED, returning 0\n");
    }
      prot_res = mprotect(v_ptr,10*sizeof(int),PROT_READ | PROT_WRITE);
    }
  }
}