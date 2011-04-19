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

Safepoint_Ability::Safepoint_Ability(bool is_a) {
  prev = The_Squeak_Interpreter()->safepoint_ability;
  _is_able = is_a;
  The_Squeak_Interpreter()->safepoint_ability = this;
}

Safepoint_Ability::~Safepoint_Ability() {
  The_Squeak_Interpreter()->safepoint_ability = prev;
}

bool Safepoint_Ability::is_interpreter_able() { return The_Squeak_Interpreter()->safepoint_ability->is_able(); }
