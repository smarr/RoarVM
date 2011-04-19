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


class Bitmap {
  // state

  int   _bit_length;
  int   _map_length;
  typedef char map_t;  static const int map_elem_shift = 3;
  map_t* _map;

  static const int map_elem_byte_size = sizeof(map_t);
  static const int map_elem_bit_size = map_elem_byte_size * 8;
  static map_t mask_for(int bit_index) { return 1 << (bit_index & (map_elem_bit_size - 1)); }

  static int map_index(int bit_index) { return bit_index >> map_elem_shift;  }
  static int map_length(int bit_length) { return map_index(bit_length - 1) + 1; }


public:
  // constructors / destructors / growing
  Bitmap(int bit_length) {
    assert_eq((1 << map_elem_shift),  map_elem_bit_size, "should be log");
    assert_message((int)sizeof(int) >= map_elem_byte_size, "mask_for needs this");
    _bit_length = bit_length;
    _map_length = map_length(_bit_length);
    _map = new char[_map_length];
    bzero(_map, _map_length);
  }

  ~Bitmap() { delete[] _map; }

private:

  void grow_if_needed(int bit_index) {
    if (bit_index < _bit_length)  return;
    int required_bit_length = bit_index + 1;
    int new_bit_length = max(required_bit_length,  _bit_length * 2);
    int new_map_length = map_length(new_bit_length);
    int new_map_byte_length = new_map_length * map_elem_byte_size;
    int map_byte_length = _map_length * map_elem_byte_size;

    map_t* new_map = (map_t*)new char[new_map_byte_length];
    memcpy(new_map, _map, map_byte_length);
    memset((void*)((intptr_t)new_map + map_byte_length),  0,  new_map_byte_length - map_byte_length);

    delete[] _map;
    _bit_length = new_bit_length;
    _map_length = new_map_length;
    _map        = new_map;
  }

  map_t& element_for(int bit_index) {
    grow_if_needed(bit_index);
    return _map[map_index(bit_index)];
  }

public:
  // accessing

  bool is_set(int i) { return element_for(i) & mask_for(i); }

  bool is_set_bool(int i) { return !(!is_set(i));  } // returns 0 or 1

  // mutating

  void set(int i, bool b) { b ? set(i) : clear(i); }
  void clear(int i) { element_for(i) &= ~mask_for(i); }
  void set  (int i) { element_for(i) |=  mask_for(i); }

  void clear_all() { memset(_map, 0, _map_length * map_elem_byte_size); }

  void ensure_clear_then_set(int i) {
    map_t& b = element_for(i);  char m = mask_for(i);
    assert_always(!(b & m));
    b |= m;
  }
  static void test();
};

