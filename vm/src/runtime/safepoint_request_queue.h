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


class Safepoint_Request_Queue {
  struct elem {
    int rank;
    const char* why;
    int sequence_number; 
  };
  elem *contents;
  int _size;
  int _occupancy;
  int _next_free;
  
 public:
  bool is_empty() { return _occupancy <= 0; }
 private:
  bool is_full()  { return _occupancy >= _size; }
  
  int oldest() {
    if (is_empty())  fatal("empty");
    return  (_next_free + _size - _occupancy)  %  _size;
  }
  
  void advance(int& i) { ++i; i %= _size; }
  
  elem* oldest_elem() {
    if (is_empty()) fatal("empty");
    return &contents[oldest()];
  }

  public:
  static void unit_test();

  Safepoint_Request_Queue(int sz) {
    _size = sz;
    contents = new elem[sz];
    _occupancy = _next_free = 0;
  }

  void add(int x, const char* why, int seq_no) {
    if (is_full()) fatal("out of space");
    contents[_next_free].rank = x;  contents[_next_free].why = why;  
    contents[_next_free].sequence_number = seq_no;
    ++_occupancy;  advance(_next_free);
  }

  int oldest_rank() { return oldest_elem()->rank; }
  int oldest_sequence_number() { return oldest_elem()->sequence_number; }
  const char* oldest_why()  { return oldest_elem()->why; }
  
  void remove() {
    if (is_empty()) fatal("empty");
    --_occupancy;
  }


  void print_string(char* buf, int buf_size);
};

