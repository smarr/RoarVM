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
  void setFullScreenFlag(int32 value);
  int32 getFullScreenFlag(void);
  
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
  int     ioSetCursorWithMask(char*, char*, int, int);
  void    ioSetInputSemaphore(int);
  int32   ioSeconds();
  int    ioSetDisplayMode(int, int, int, int);
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
  
  
 void basic_init();
 void set_num_cores(char* num_cores_str);
 void go_parallel();
  void interpret_rvm(char* image_path);

}

extern   int (*compilerHooks[])();

