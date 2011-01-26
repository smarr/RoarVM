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


static const size_t CACHELINE_SIZE = 64;

template <typename T>
union cacheline_aligned {                                     
  T value;
  char alignment[(CACHELINE_SIZE > sizeof(T))
                 ? CACHELINE_SIZE
                 : (CACHELINE_SIZE * 2 > sizeof(T))
                 ? CACHELINE_SIZE * 2
                 : (CACHELINE_SIZE * 3 > sizeof(T))
                 ? CACHELINE_SIZE * 3
                 : CACHELINE_SIZE * 4];
#warning STEFAN: do not have a better idea how to do that here... \
                 but clearly that is not fit for arbitrary data types
};