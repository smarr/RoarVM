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


# include "headers.h"
# include "sys/time.h"
#include <time.h>

void wordset( int32* start, int32 contents, int n) {
  for (int i = 0;  i < n;  start[i++] = contents)
    ;
}


int round_up_to_power_of_two(int x) {
  if (x == 0)  return x;
  if (OS_Interface::population_count(x) == 1)  return x;
  int lz = OS_Interface::leading_zeros(x);
  int number_of_bits_after_leading_zeros = 32 - lz;
  int result = 1 << number_of_bits_after_leading_zeros;
  assert_always((result >> 1) < x  &&  x <= result);
  return result;
}


int log_of_power_of_two(int x) {
  int lz = OS_Interface::leading_zeros(x);
  int number_of_bits_after_leading_zeros = 32 - lz;
  int result = number_of_bits_after_leading_zeros - 1;
  assert_always((1 << result) == x);
  return result;
}

void print_time() {
  struct timeval tv;
  if (gettimeofday(&tv, NULL))
    fprintf(stderr, "gettimeofday failed\n");
  fprintf(stderr, "%s", ctime(&tv.tv_sec));
}

int least_significant_bit_position(u_int64 x) {
  for (int i = 0;  i < 64;  ++i, (x >>= 1))
    if (x & 1) return i;
  return 64;
}



extern "C" void lprintf(const char* msg, ...) {
  va_list ap;
  va_start(ap, msg);
  vlprintf(msg, ap);
  va_end(ap);
}

void vlprintf(const char* msg, va_list ap) {
  FILE* f = stderr;
  Squeak_Interpreter* const interp = The_Squeak_Interpreter();
  fprintf(f, "%d on %d (%d): ",
          (interp) ? interp->increment_print_sequence_number() : -1,
          Logical_Core::my_rank(),
          getpid());
  vfprintf(f, msg, ap);
}

