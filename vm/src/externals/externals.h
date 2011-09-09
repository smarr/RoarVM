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


extern "C" {
  int32 vmPathSize();
  void  vmPathGetLength(char*, int);
  void  sqGetFilenameFromString(char*, char*, int, int);

  void rvm_callInitializersInAllModules();
}


extern "C" {
# if On_iOS
  void setFullScreenFlag(int32 value);
  int32 getFullScreenFlag(void);
# endif

  int sqr_main(int, char**, char**);


  void* dummy_fn(...);


  void    ioBeep();
  int     ioExit();
  void    ioForceDisplayUpdate();
  bool    ioFormPrint(int32* bitsAddr, int w, int h, int depth, double hScale, double vScale, bool landscapeFlag);
  int     ioGetButtonState();
  int     ioGetNextEvent(void*);
  bool    ioHasDisplayDepth(int depth);
  char*   ioListBuiltinModule(int);
  char*   ioListExternalModule(int);
  fn_t    ioLoadExternalFunctionOfLengthFromModuleOfLength(char*, int, char*, int);
  fn_t    ioLoadFunctionFrom(const char*, const char*);
  int32   ioMousePoint();
  int     ioMSecs();
  void    ioProcessEvents();
  void    ioRelinquishProcessorForMicroseconds(int);
  u_int32 ioScreenSize();
  void    ioSetCursor(char*, int, int);
# if On_iOS
# define T int
# else
# define T void
# endif
  T    ioSetCursorWithMask(char*, char*, int, int);
# undef T
  void    ioSetInputSemaphore(int);
  int32   ioSeconds();
# if On_iOS
# define T int
# else
# define T bool
# endif
  T    ioSetDisplayMode(int, int, int, int);
# undef T
  void    ioSetFullScreen(bool);
  void    ioShowDisplay(char*, int, int, int,   int, int, int, int);

  int     ioGetKeystroke();
  int     ioPeekKeystroke();



  void clipboardWriteFromAt(int, char*, int);
  int  clipboardSize();
  void clipboardReadIntoAt(int, int, char*);


  void getAttributeIntoLength(int id, char* p, int len);
  int attributeSize(int id);



  int setCompilerInitialized(int flagValue);

  void sigint();
  
# if On_iOS
  /* Will be used from Obj-C code to initalize the VM */
  void initialize_basic_subsystems();
  void set_num_cores(char* num_cores_str);
  void initialize_interpreter_instances_selftest_and_interpreter_proxy(char** orig_argv);
  void read_image(char* image_path);
  void begin_interpretation();
# endif

}

extern   int (*compilerHooks[])();



int event_type_complex();


