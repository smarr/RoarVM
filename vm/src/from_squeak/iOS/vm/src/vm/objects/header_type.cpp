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


char Header_Type::extra_bytes_without_preheader[4] = {
  bytesPerWord * 2  , // "3-word header (type 0)"
  bytesPerWord      ,	// "2-word header (type 1)"
  0                 ,	// "free chunk (type 2)"
  0                   // "1-word header (type 3)"
};

char Header_Type::extra_bytes_with_preheader[4] = {
  bytesPerWord * 2  +  preheader_byte_size, // "3-word header (type 0)"
  bytesPerWord      +  preheader_byte_size,	// "2-word header (type 1)"
  0                 +  0,	// "free chunk (type 2)"
  0                 +  preheader_byte_size  // "1-word header (type 3)"
};



char Header_Type::extra_oops_without_preheader[4] = {
  2  , // "3-word header (type 0)"
  1  ,	// "2-word header (type 1)"
  0  ,	// "free chunk (type 2)"
  0     // "1-word header (type 3)"
};

char Header_Type::extra_oops_with_preheader[4] = {
  2  +  preheader_oop_size, // "3-word header (type 0)"
  1  +  preheader_oop_size,	// "2-word header (type 1)"
  0  +  0,	// "free chunk (type 2)"
  0  +  preheader_oop_size  // "1-word header (type 3)"
};

