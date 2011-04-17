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

Printer*          stdout_printer;
Printer*          stderr_printer;
Printer*           debug_printer;
Printer*           error_printer;
Printer* dittoing_stdout_printer;

void Printer::init_globals() {

           stdout_printer = new FilePointerPrinter(stdout);
           stderr_printer = new FilePointerPrinter(stderr);
            debug_printer = new FilePointerPrinter(stdout, true);
            error_printer = new FilePointerPrinter(stderr, true);
  dittoing_stdout_printer = new DittoingPrinter(stdout_printer);
}


void Printer::printf(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  print_to_wherever(fmt, ap);
  va_end(ap);
}



void Printer::lprintf(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  printf("%d on %d: ",
         The_Squeak_Interpreter()->increment_print_sequence_number(),
         Logical_Core::my_rank());
  print_to_wherever(fmt, ap);
  va_end(ap);
}


void FilePointerPrinter::print_to_wherever(const char* fmt, va_list ap) {
  // just stdout for now
  vfprintf(fp, fmt, ap);
  // for sanity's sake, keep 'em synced
  if (fp == stdout) fflush(stderr);  if (fp == stderr) fflush(stdout);
  fflush(fp);
}

void StringPrinter::print_to_wherever(const char* fmt, va_list ap) {
  if (used_len_without_null < buf_len - 1) {
    int n = vsnprintf(buf + used_len_without_null, buf_len - used_len_without_null - 1 /* for null*/, fmt, ap);
    used_len_without_null += n /*- 1 Stefan this -1 seems to be wrong, always cuts of last char*/  /* null */;
  }
}


void DittoingPrinter::print_to_wherever(const char* fmt, va_list ap) {
  char buf[buf_len];
  /*int n =*/ vsnprintf(buf, buf_len - 1, fmt, ap);
  if (!allow_dittoing ||  strncmp(buf, last_line, buf_len) != 0) {
    if (dittoing) {
      dittoing = false;
      real_printer->printf("\n");
    }
    real_printer->printf("%s", buf);
    snprintf(last_line, sizeof(last_line), "%s", buf);
  }
  else {
    dittoing = true;
    real_printer->printf("\"");
  }
}

