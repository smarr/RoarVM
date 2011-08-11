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


inline int round_up_by_power_of_two(int x, int power_of_two) {
  return (x + power_of_two - 1) & ~(power_of_two - 1);
}

inline int divide_by_power_of_two_and_round_up(int x, int power_of_two) {
  return round_up_by_power_of_two(x, power_of_two) / power_of_two;
}

inline int divide_and_round_up(int x, int y) {
  return (x + y - 1) / y;
}
inline int round_up(int x, int y) {
  return divide_and_round_up(x, y) * y;
}


# define min(a,b) ((a) <= (b)  ?  (a)  :  (b))
# define max(a,b) ((a) >= (b)  ?  (a)  :  (b))

inline void swap_bytes_long(int32* p) {
  *p = (*p << 24)  |  ((*p << 8) & 0xff0000)  |  ((*p >> 8) & 0xff00)  |  ((*p >> 24) & 0xff);
}
inline void reverseBytes(int32* start, int32* stop) {
  fprintf(stdout, "reversing bytes\n");

  for (int32* p = start;  p < stop;  ++p)
    swap_bytes_long(p);

  fprintf(stdout, "done reversing bytes\n");
}


inline bool is_vowel(char c) {
  switch ( tolower(c) ) {
    case 'a':
    case 'e':
    case 'i':
    case 'o':
    case 'u':
      return true;
    default: return false;
  }
}

void wordset( int32*, int32, int n);

inline int xfread(void* p, int sz, int n, FILE* f) {
  int r = fread(p, sz, n, f);
  if (r != n) { fprintf(stdout, "Expected to read %d bytes but only read %d bytes...\n", n, r); fatal("xfread"); }
  return r;
}

inline int xfwrite(const void* p, int sz, int n, FILE* f) {
  int r = fwrite(p, sz, n, f);
  if (r != n) { fatal("xfwrite"); }
  return r;
}


inline void write_mark(FILE* f, const char* m) { if (check_assertions) xfwrite(m, 4, 1, f); }
inline void read_mark(FILE* f, const char* m) {
  if (!check_assertions) return;
  int mm;
  xfread(&mm, sizeof(mm), 1, f);
  if (mm != *(int*)m) fatal("mark mismatch");
}

int round_up_to_power_of_two(int);
int log_of_power_of_two(int);
int least_significant_bit_position(u_int64);

void print_time();


extern "C" void lprintf(const char* msg, ...);
void vlprintf(const char* msg, va_list ap);

