/******************************************************************************
 *  Copyright (c) 2008 - 2011 IBM Corporation and others.
 *  All rights reserved. This program and the accompanying materials
 *  are made available under the terms of the Eclipse Public License v1.0
 *  which accompanies this distribution, and is available at
 *  http://www.eclipse.org/legal/epl-v10.html
 * 
 *  Contributors:
 *    Mattias De Wael, Vrije Universiteit Brussel - Parallel Garbage Collection
 *    Wouter Amerijckx, Vrije Universiteit Brussel - Parallel Garbage Collection
 ******************************************************************************/


bool on_NMT_trap(Oop* p, Oop value);

void on_Protected_trap(Oop* p, Oop oldValue);

bool is_pointing_to_protected_page_slowVersion(Oop oop);

bool is_pointing_to_protected_page(Oop oop);

void doLVB(Oop* p);
