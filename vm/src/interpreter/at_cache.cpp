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

void At_Cache::Entry::install(Oop rcvr, bool stringy) {
  Object_p ro = rcvr.as_object();
  int rcvr_fmt = ro->format();
  if (Object::Format::might_be_context(rcvr_fmt) && ro->hasContextHeader()) {
    The_Squeak_Interpreter()->primitiveFail();
    return;
  }
  oop_int_t totalLength = ro->lengthOf();
  oop_int_t rcvr_fixedFields = ro->fixedFieldsOfArray();

  oop = rcvr;
  fmt = stringy ? rcvr_fmt + 16 : rcvr_fmt;
  fixedFields = rcvr_fixedFields;
  size = totalLength - rcvr_fixedFields;
}

