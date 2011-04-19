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

void Bitmap::test() {
  Bitmap b(5);
  for (int i = 0; i < 100;  i += 7)  b.set(i);
  for (int i = 0; i < 100;  ++i)
    assert_always(b.is_set_bool(i) == (i % 7  ==  0));
  for (int i = 0;  i < 100;  ++i) b.set(i);
  for (int i = 0; i < 100;  i += 7)  b.clear(i);
  for (int i = 0; i < 100;  ++i)
    assert_always(b.is_set_bool(i) != (i % 7  ==  0));
}

