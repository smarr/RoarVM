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


# if   On_Tilera
  # if On_Tilera_With_GCC
  class   TMC_OS_Interface;
  typedef TMC_OS_Interface  OS_Interface;
  # else
  class   ILib_OS_Interface;
  typedef ILib_OS_Interface  OS_Interface;
  # endif
# elif On_Apple

  class   OSX_OS_Interface;
  typedef OSX_OS_Interface   OS_Interface;

# else

  class   POSIX_OS_Interface;
  typedef POSIX_OS_Interface OS_Interface;

# endif
