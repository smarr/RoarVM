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


class Squeak_Image_Reader {
  friend class Convert_Closure;
 private:
  char* file_name;
  FILE* image_file;
  Memory_System* memory_system;
  Squeak_Interpreter* interpreter;

  u_int32 headerStart, headerSize, dataSize, extraVMMemory;

  char *oldBaseAddr, *memory;
  Oop* object_oops;


  // need to be passed on
  Oop specialObjectsOop; // -> The_Squeak_Interpreter()->roots.specialObjectsOop
  int32 lastHash;
  int32 savedWindowSize;
  int32 fullScreenFlag;


  bool swap_bytes;
  void check_image_version();
  int32 get_long();
  static bool readable_format(int32);
  bool is_cog_image_with_reodered_floats();
  static int32 image_format_version();
  void read_header();

  void byteSwapByteObjects();
  void normalize_float_ordering_in_image();
  void distribute_objects();
  
  void complete_remapping_of_pointers();

public:
  static void imageNamePut_on_all_cores(char*  b, unsigned int n);
  Oop oop_for_oop(Oop);
private:
  Oop oop_for_addr(Object*);
  Oop oop_for_relative_addr(int);

  Squeak_Image_Reader(char* file_name, Memory_System* , Squeak_Interpreter* i);

 public:
  static void read(char*, Memory_System* h, Squeak_Interpreter* i);
  static void fake_read(char*, Memory_System* h, Squeak_Interpreter* i);
  void read_image();
  
  static const int Pre_Closure_32_Bit_Image_Version = 6502;
  static const int Post_Closure_32_Bit_Image_Version = 6504;
  static const int Post_Closure_With_Reordered_Floats_32_Bit_Image_Version = 6505;
 };

