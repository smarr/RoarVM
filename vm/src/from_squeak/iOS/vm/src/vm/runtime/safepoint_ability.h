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


// Is the interpreter in a state where another core could move objects around?
// Stack-allocated

// CAVEAT: where ever the code says Safepoint_Ability sa(true),
// there may be a GC if any messages are sent.
// You much look all the way up the call stack to ensure there are no Object* local variables,
// because a GC will cause these to be incorrect! They need to be recalculated from Oops after the GC.
// If you wanted to be really careful, you would push the oops on the mapped oop stack in the interpreter,
// in case a future GC algorithm actually changes the Oops. -- dmu 10/1/10
// REM: for this reason we are using now Object_p instead of plain Object*, this
// will allow us to identify such invalid uses in debug mode.

class Safepoint_Ability {
  bool _is_able;
  Safepoint_Ability* prev;
public:
  bool is_able()   { return _is_able; }
  bool is_unable() { return !is_able(); }
  void be_unable() { _is_able = false; }

  Safepoint_Ability(bool);
  ~Safepoint_Ability();
  void* operator new(size_t) { fatal("should be stack-allocated"); return (Safepoint_Ability*)NULL; }
  static bool is_interpreter_able();
};
