#define TRAPPED 666
# if !On_Apple
#include <ucontext.h>
# endif

#define SIGNAL_ACCES_PROTECTED_PAGE     SIGSEGV
#define PRINT_REASON(info)\
switch (info->si_code) {\
case SEGV_MAPERR:\
printf("Address not mapped.\n");\
break;\
case SEGV_ACCERR:\
printf("Invalid permissions.\n");\
break;\
default:\
break;\
}

void signal_handler_setEax(int sig, siginfo_t *info, ucontext_t *uap);

int install_signalhandler(int signum,  void* sig_handler);

int install_signalhandler_protectedPageAcces();

void TEST_force_protectedPage_signal_trap(void* p);

void TEST_force_protectedPage_signal_trap();
