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


Oop Abstract_Tracer::get() {
  int n = wrapped  ?  size  :  next;
  Object* r = array_class().as_object()->instantiateClass(n * elem_gotten_elem_size);
  void* p = r->firstIndexableField_for_primitives();

  if (!wrapped)
    copy_elements(0, p, 0, n, r);
  else {
    copy_elements( next,  p,            0,  size - next,  r);
    copy_elements(    0,  p,  size - next,         next,  r);
  }
  next = 0; wrapped = false;
  return r->as_oop();
}

