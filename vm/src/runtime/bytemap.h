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


class Bytemap {
  // state

  int   _map_length;
  char* _map;


public:
  // constructors / destructors / growing
  Bytemap(int len) {
    _map_length = len;
    _map = new char[_map_length];
  }

  ~Bytemap() { delete[] _map; }

private:

  void grow_if_needed(int len) {
    if (len < _map_length)  return;
    int new_len = max(len, 2 * _map_length);

    char* new_map = new char[new_len];
    memcpy(new_map, _map, _map_length);
    memset(new_map + _map_length,  0,  new_len - _map_length);

    delete[] _map;
    _map_length = new_len;
    _map        = new_map;
  }

  char& byte_for(int i) {
    grow_if_needed(i);
    return _map[i];
  }

public:

  // mutating

  void set(int i, char c) { byte_for(i) = c; }
  char get(int i) { return byte_for(i); }
  void clear_all() { memset(_map, 0, _map_length); }

  void ensure_clear_then_set(int i, char c) {
    char& b = byte_for(i);
    assert_always(!b);
    b = c;
  }
  static void test();
};

