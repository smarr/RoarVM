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


extern class Obsolete_Named_Primitive_Table {
public:
  struct entry { const char* oldName; const char* plugin; const char* newName; bool on_main; };
  static entry contents[];
  int find(char* name, int name_len, char* plugin, int plugin_len);
} obsoleteNamedPrimitiveTable;

