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


class Tile_CPU_Coordinate : public Abstract_CPU_Coordinate {
public:
  
  bool print(char* buf, int buf_size) {
    snprintf(buf, buf_size, "%d, %d", _my_x, _my_y);
    return true;
  }
  
  
  
  static int _my_x, _my_y;
  static int center_x, center_y;               // threadsafe, read only after init, Stefan: 2009-09-06
  static int center_rank;                      // threadsafe, read only after init, Stefan: 2009-09-06
  static int main_x, main_y;                   // threadsafe, read only after init, Stefan: 2009-09-06
  
  static  int width, height;                   // threadsafe, read only after init, Stefan: 2009-09-06
  static void set_width_height(int w, int h);  // threadsafe, read only after init, Stefan: 2009-09-06

  
  static bool is_center() { return _my_x == center_x  &&  _my_y == center_y; }
  static int my_x() { return _my_x; }
  static int my_y() { return _my_y; }

  static void initialize_all_cores();
         void initialize(int rank);
  static bool is_initialized() { return true; }


  static Oop get_stats();
  
private:
  static void really_set_width_height(int, int);
};

