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


#include "headers.h"

void Safepoint_Request_Queue::unit_test() {
  Safepoint_Request_Queue q1(3);
  assert_always(q1.is_empty());

  q1.add(3, "a", 1);
  assert_always(!q1.is_empty());
  q1.add(4, "b", 2);
  assert_always(!q1.is_empty());
  q1.add(5, "c", 3);
  assert_always(!q1.is_empty());
  // should fail! q1.add(6, "d");

  assert_always(q1.oldest_rank() == 3  &&  strcmp(q1.oldest_why(), "a") == 0);
  q1.remove();
  assert_always(!q1.is_empty());
  assert_always(q1.oldest_rank() == 4  &&  strcmp(q1.oldest_why(), "b") == 0);
  q1.remove();
  assert_always(!q1.is_empty());
  assert_always(q1.oldest_rank() == 5  &&  strcmp(q1.oldest_why(), "c") == 0);
  q1.remove();
  assert_always(q1.is_empty());
  // q1.remove(); // should fail

  q1.add(17, "d", 4);
  q1.add(18, "e", 5);
  q1.add(19, "f", 6);
  assert_always(q1.oldest_rank() == 17  &&  strcmp(q1.oldest_why(), "d") == 0); q1.remove();
  q1.add(20, "g", 7);
  assert_always(q1.oldest_rank() == 18  &&  strcmp(q1.oldest_why(), "e") == 0); q1.remove();
  q1.add(21, "h", 8);

  assert_always(q1.oldest_rank() == 19  &&  strcmp(q1.oldest_why(), "f") == 0); q1.remove();
  assert_always(q1.oldest_rank() == 20  &&  strcmp(q1.oldest_why(), "g") == 0); q1.remove();
  assert_always(q1.oldest_rank() == 21  &&  strcmp(q1.oldest_why(), "h") == 0); q1.remove();
  assert_always(q1.is_empty());

}


void Safepoint_Request_Queue::print_string(char* buf, int buf_size) {
  if (is_empty()) {
    strncpy(buf, "<empty>", buf_size);
    return;
  }
  char *p = buf;
  for (int i = oldest(), elems_to_do = _occupancy; 
       elems_to_do;  
       advance(i), --elems_to_do) {
    int n = snprintf(p, buf_size - (p - buf),
                     "%d<%d>(%s)%s",
                     contents[i].rank, contents[i].sequence_number, contents[i].why, elems_to_do > 1  ? ", " : "");
    p += n;
    if (p - buf >= buf_size) fatal("buf too small");
  }
  return;
}

