#ifndef CHECK_FOR_PROBLEMATIC_COMPILER
  #define CHECK_FOR_PROBLEMATIC_COMPILER 1
#endif

#if CHECK_FOR_PROBLEMATIC_COMPILER

#ifdef __GNUC__
  #if __GNUC__ == 4 && __GNUC_MINOR__ > 2
     #error Your compiler causes trouble with the current RVM version. \
            Some compiler optimizations lead to crashes. \
            This is probably caused by a bug we have not managed \
            to identify yet. \
            To disable this check define CHECK_FOR_PROBLEMATIC_COMPILER as 0. \
            This can be done by using './configure --opt-workaround' which will\
            pratially adapt the optimization level.
  #endif
#endif

#ifdef __INTEL_COMPILER
   #error Your compiler causes trouble with the current RVM version. \
          Some compiler optimizations lead to crashes. \
          This is probably caused by a bug we have not managed \
          to identify yet. \
          To disable this check define CHECK_FOR_PROBLEMATIC_COMPILER as 0. \
          This can be done by using './configure --opt-workaround' which will \
          pratially adapt the optimization level.

#endif

#endif // CHECK_FOR_PROBLEMATIC_COMPILER
            
