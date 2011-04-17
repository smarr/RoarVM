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


#include <pthread.h>

# define Max_Number_Of_Cores 64

# include <stdio.h>
# include <stdint.h>
# include <stdlib.h>
# include <stdarg.h>

# if On_iOS
# elif On_Apple
#  include <monitor.h>
#  include <libkern/OSAtomic.h>
# else
#  include <sys/resource.h>
# endif

# include "tags.h"
# include "error_handling.h"
# include "printer.h"
# include "types.h"
# include "os_interface.h"
# include "abstract_os_interface.h"
# include "ilib_os_interface.h"
# include "posix_os_interface.h"
# include "osx_os_interface.h"
