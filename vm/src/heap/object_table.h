/******************************************************************************
 *  Copyright (c) 2008 - 2011 IBM Corporation and others.
 *  All rights reserved. This program and the accompanying materials
 *  are made available under the terms of the Eclipse Public License v1.0
 *  which accompanies this distribution, and is available at
 *  http://www.eclipse.org/legal/epl-v10.html
 * 
 *  Contributors:
 *    Stefan Marr, Vrije Universiteit Brussel - Initial Implementation
 ******************************************************************************/


# if   Use_Object_Table

class   Multicore_Object_Table;
typedef Multicore_Object_Table  Object_Table;

# else

class   Dummy_Object_Table;
typedef Dummy_Object_Table      Object_Table;

# endif
