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


// A class for printing that can be subclassed to print to a string, a file, etc.
class Printer {
 public:
  static void init_globals();
  Printer(bool iid = false) { include_implementation_details = iid; }
  virtual ~Printer() {}
  bool include_implementation_details;
  void printf(const char* fmt, ...);
  void lprintf(const char* fmt, ...);
  void nl() { printf("\n"); }
  virtual void dittoing_off() {}
  virtual void dittoing_on() {}
 private:
  virtual void print_to_wherever(const char* fmt, va_list) = 0;
};

class FilePointerPrinter: public Printer {
 private:
  FILE* fp;
 public:
  FilePointerPrinter(FILE* x, bool iid = false) : Printer(iid) {fp = x;}
  void print_to_wherever(const char* fmt, va_list);
};

class StringPrinter: public Printer {
 private:
  char* buf;
  int buf_len;
  int used_len_without_null;
  bool dittoing;
 public:
  StringPrinter(char* b, int n, bool iid = false) : Printer(iid) {
    buf = b; buf_len = n;  used_len_without_null = 0;}
  void print_to_wherever(const char* fmt, va_list);
};

// only works if printf's are for whole lines at a time
class DittoingPrinter: public Printer {
 private:
  Printer* real_printer;
  static const int buf_len = 10000;
  char last_line[buf_len];
  bool dittoing;
  bool allow_dittoing;
 public:
  DittoingPrinter(Printer* rp, bool iid = false) : Printer(iid) {
    real_printer = rp;  dittoing = false;  last_line[0] = '\0';  allow_dittoing = true;
  }
  void print_to_wherever(const char* fmt, va_list);
  void dittoing_off() { allow_dittoing = false; }
  void dittoing_on()  { allow_dittoing = true; }
};


extern Printer *debug_printer, *error_printer, *stdout_printer, *stderr_printer, *dittoing_stdout_printer;

