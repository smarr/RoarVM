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


class GC_Oop_Stack {
  static const int N = 10000;
  struct Contents {
    Object* objs[N];
    Contents* next_contents;
    Contents(Contents* last) { next_contents = last; }
    ~Contents() { if (next_contents != NULL) { delete next_contents; next_contents = NULL;  } }
 } *contents, *free_contents;
  int next_elem;

public:
   GC_Oop_Stack() { contents = NULL;  free_contents = NULL; next_elem = N; }
    ~GC_Oop_Stack() {
      if (contents != NULL) { delete contents; contents = NULL;  }
      if (free_contents != NULL) { delete free_contents; free_contents = NULL; }
    }

  void push(Object* x) {
    if (next_elem < N)
      contents->objs[next_elem++] = x;
    else {
      if (free_contents == NULL)
        contents = new Contents(contents);
      else {
        Contents* c = free_contents;
        free_contents = c->next_contents;
        c->next_contents = contents;
        contents = c;
      }
      next_elem = 0;
      push(x);
    }
  }

  bool is_empty() { return next_elem == N  &&  contents == NULL; }

  Object* pop() {
    Object* r = contents->objs[--next_elem];
    if (next_elem == 0) {
      next_elem = N;
      if (contents != NULL) {
        Contents* c = contents;
        contents = c->next_contents;

        c->next_contents = free_contents;
        free_contents = c;
      }
    }
    return r;
  }
};

