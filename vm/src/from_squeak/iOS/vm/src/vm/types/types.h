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


typedef long int      int32;
typedef long long int int64;
typedef short int     int16;

typedef unsigned int           u_int1;   // used for 1-bit fields, Tilera compiler complains if its not unsigned
typedef unsigned long int      u_int32;
typedef unsigned long long int u_int64;

typedef unsigned char u_char;

typedef   int32   oop_int_t;
typedef u_int32 u_oop_int_t;

typedef void* (*fn_t)(...);


class Object;
class Chunk;
class Abstract_Mark_Sweep_Collector;
class Squeak_Image_Reader;
class Squeak_Interpreter;

class typedefs {
 public:
  static void check_typedefs() {
    assert_eq(sizeof(u_int32), 4, "");
    assert_eq(sizeof(int32), 4, "");
    assert_eq(sizeof(int16), 2, "");
    assert_eq(sizeof(int64), 8, "");
    assert_eq(sizeof(u_int64), 8, "");
    debug_printer->printf("typedefs::check_assertions passed\n");
  }

};

static const int bytesPerWord = sizeof(int32);
static const int ShiftForWord = 2;
static const int BitsPerByte = 8;
static const int BitsPerWord = sizeof(oop_int_t) * BitsPerByte;
static const int BitsPerSmallInt = BitsPerWord - Tag_Size;

static const int MinSmallInt = (1 << BitsPerSmallInt);
static const int MaxSmallInt = ~MinSmallInt;

static const int Mega = 1024 * 1024;

