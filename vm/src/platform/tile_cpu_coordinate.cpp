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

# if On_Tilera

int Tile_CPU_Coordinate::_my_x = -1;
int Tile_CPU_Coordinate::_my_y = -1;

void Tile_CPU_Coordinate::initialize(int rank) {
  x = x_of_rank(rank);
  y = y_of_rank(rank);
  if (rank == Logical_Core::my_rank()  &&  (x != _my_x || y != _my_y))
    fatal("x and y?");
}


int Tile_CPU_Coordinate::width;
int Tile_CPU_Coordinate::height;

int Tile_CPU_Coordinate::center_x;
int Tile_CPU_Coordinate::center_y;
int Tile_CPU_Coordinate::center_rank;

int Tile_CPU_Coordinate::main_x;
int Tile_CPU_Coordinate::main_y;




void Tile_CPU_Coordinate::set_width_height(int w, int h) {
  if (Measure_Communication) {
    if (Logical_Core::running_on_main())
      fprintf(stderr, "Measure_Communication is set; overriding your settings for width and height\n");
    really_set_width_height(1, 2);
  }
  else
    really_set_width_height(w, h);
}


void Tile_CPU_Coordinate::really_set_width_height(int w, int h) {
  width = w,  height = h;
  
  center_x = (width-1) / 2,  center_y = (height-1) / 2;
  center_rank = center_x  +  center_y * width;
  
  
  Logical_Core::main_rank = center_rank;
  main_x = center_x;  main_y = center_y;
  
  if (Logical_Core::running_on_main())
    fprintf(stderr, "width, height  ==  %d, %d\n", width, height);
}


Oop Tile_CPU_Coordinate::get_stats() {
  
  int s = The_Squeak_Interpreter()->makeArrayStart();
  int mainRank = Logical_Core::main_rank;
  int mainX = CPU_Coordinate::main_x, mainY = CPU_Coordinate::main_y;
  PUSH_POSITIVE_32_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(mainX);
  PUSH_POSITIVE_32_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(mainY);
  PUSH_POSITIVE_32_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(mainRank);
  PUSH_POSITIVE_32_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(CPU_Coordinate::width);
  PUSH_POSITIVE_32_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(CPU_Coordinate::height);
  
  int groupSize = Logical_Core::group_size;
  PUSH_POSITIVE_32_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(groupSize);
  PUSH_POSITIVE_32_BIT_INT_WITH_STRING_FOR_MAKE_ARRAY(Logical_Core::remaining);
  
  Oop groupStats = The_Squeak_Interpreter()->makeArray(s);
  PUSH_WITH_STRING_FOR_MAKE_ARRAY(groupStats);
  
  return The_Squeak_Interpreter()->makeArray(s);
}


# endif // On_Tilera

