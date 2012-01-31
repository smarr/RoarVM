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


# include "headers.h"


void print_config() {
  fprintf(stdout, "Configuration flags:\n");
  # define PRINT(name) fprintf(stdout, "\t%s%s\n", name ? " " : "!", #name);
  DO_ALL_CONFIG_FLAGS(PRINT)
  fprintf(stdout, "\n");
  # undef PRINT
}



void print_config_for_spreadsheet() {

  # define PRINT( name) fprintf(stdout, "%s\t%d\n", #name, name);

  DO_ALL_CONFIG_FLAGS(PRINT)
  fprintf(stdout, "\n");
}


# undef PRINT

