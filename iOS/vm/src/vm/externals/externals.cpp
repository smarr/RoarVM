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


#include "headers.h"


char* ioListExternalModule(int) {  unimpExt(); return NULL; }

int setCompilerInitialized(int flagValue) {unimpExt(); return 0; }

int (*compilerHooks[])() = {NULL};



# if 0

fn_t ioLoadFunctionFrom(const char*, const char*) {unimpExt(); return dummy_fn; }


fn_t ioLoadExternalFunctionOfLengthFromModuleOfLength(char*, int, char*, int) {
  unimpExt();
  assert_on_main();
  return NULL;
}


char* ioListBuiltinModule(int) {  unimpExt(); return NULL; }

void* dummy_fn(...) {unimplemented(); return 0;}



void ioGetNextEvent(void*) { unimpExt(); }
int ioGetButtonState() {unimpExt(); return 0; }
int32 ioMousePoint() {unimpExt(); return 0;}





bool ioSetDisplayMode(int, int, int, bool) {unimpExt();  return true;}


bool ioHasDisplayDepth(int depth) {unimpExt(); return true;}

# endif

void sigint() {The_Squeak_Interpreter()->handle_sigint();}

