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


class GC_State {
public:
  bool is_mark_phase()     { return is_phase(mark_phase_tag);     };
  bool is_relocate_phase() { return is_phase(relocate_phase_tag); };
  bool is_remap_phase()    { return is_phase(remap_phase_tag);    };
  
  
  void set_phase_mark()   { set_phase(mark_phase_tag);   };
  void unset_phase_mark() { unset_phase(mark_phase_tag); };
  
  void set_phase_relocate()   { set_phase(relocate_phase_tag);   };
  void unset_phase_relocate() { unset_phase(relocate_phase_tag); };
  
  void set_phase_remap()   { set_phase(remap_phase_tag);   };
  void unset_phase_remap() { unset_phase(remap_phase_tag); };
  int phase;
  
private:
  static const int mark_phase_tag     = 1;
  static const int relocate_phase_tag = 2;
  static const int remap_phase_tag    = 4;
  
  /* Inspect current running GC-phase(s) */
  bool is_phase(int phase_tag) { return (phase & phase_tag); };
  
  
  void set_phase(int phase_tag)   { phase |= phase_tag; };
  void unset_phase(int phase_tag) { phase &= ~phase_tag; };
};
