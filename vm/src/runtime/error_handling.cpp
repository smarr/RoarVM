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

ON_TILERA_OR_ELSE(int, void) assert_failure(const char* func, const char* file, const int line, const char* pred, const char* msg) {
   const char* safe_func = On_Tilera ? "<unknown>" : func;

  if (Memory_Semantics::cores_are_initialized())
    error_printer->printf("%s: file %s, line %d, function %s, predicate %s, rank %d, main_rank %d, pid %d\n",
        msg, file, line, safe_func, pred, Logical_Core::my_rank(), Logical_Core::main_rank, getpid());
  else
    fprintf(stderr, "%s: file %s, line %d, function %s, predicate %s, pid %d\n",
                          msg, file, line, safe_func, pred, getpid());

  OS_Interface::abort();
  ON_TILERA_OR_ELSE(return 0, );
}

ON_TILERA_OR_ELSE(int, void) assert_eq_failure(const char* func, const char* file, const int line, const char* pred, const char* msg, void* a, void* b) {
  static char buf[10000];   // threadsafe? does not really matter here anymore...
  error_printer->printf("%s: file %s, line %d, function %s, predicate %s, pid %d, 0x%x != 0x%x",
          msg, file, line, func, pred, getpid(), a, b);
  error_printer->printf("%s\n", buf);
  
  OS_Interface::abort();
  ON_TILERA_OR_ELSE(return 0, );
}

ON_TILERA_OR_ELSE(int, void) assert_eq_failure(const char* func, const char* file, const int line, const char* pred, const char* msg, int a, int b) {
  assert_eq_failure(func, file, line, pred, msg, (void*)a, (void*)b);
  ON_TILERA_OR_ELSE(return 0, );
}

void breakpoint() {
  OS_Interface::breakpoint();
  dittoing_stdout_printer->printf("breakpoint\n");
}

void unt(const char* m, const char* f) {
  dittoing_stdout_printer->printf("%s: %s\n", m, f);
}


void unte(const char* m, const char* f) {
  dittoing_stdout_printer->printf("%s: %s\n", m, f);
}

