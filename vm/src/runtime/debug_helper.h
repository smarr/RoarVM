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


void dp(Oop x); // print Oop
void dp(int x); // print Oop
void dp(Object* x); // print Object
void dp(const tracked_ptr<Object>& x); // print Object_p

void pat(); // print all stack traces
void pet(); // print execution trace
void pst(); // print stack trace

extern "C" { int printCallStack(); }
