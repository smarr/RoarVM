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

# if Multiple_Tileras


Host_PCI_Info::Host_PCI_Info() {
  read_info_file();
}



void Host_PCI_Info::read_info_file() {
# define init_info_var(name)  name = -1;
  FOR_ALL_INFOS(init_info_var)
# undef init_info_var

  
  static const char info_file_name[] = "/dev/hostpci/info";
  
  FILE* f = fopen(info_file_name, "r");
  if (f == NULL) { perror(info_file_name); fatal(""); }
  
  for (;;) {
    int r;
    char key[BUFSIZ];
    int val;
    r = fscanf(f, "%s %d ", key, &val);
    // lprintf("r %d, key %s, val %d\n", r, key, val);
    if (r != 2) break;
    
# define match_info(info_name)  else if (strcmp(#info_name, key) == 0) { info_name = val; } 
    
    if (false) ;
    FOR_ALL_INFOS(match_info)
# undef match_info
    else {
      lprintf("Encountered unknown info value in %s: %s. Add it to FOR_ALL_INFOS.\n", info_file_name, key);
      fatal("");
    }
  }
  fclose(f);
  
# define check_info(info_name) \
if (info_name < 0) fatal("uninitialiaized value was not found in info file: " #info_name); \
  FOR_ALL_INFOS(check_info)
# undef check_info
    
}


void Host_PCI_Info::print() {
# define print_info(info_name) if (Logical_Core::running_on_main()) lprintf(#info_name " = %d\n", info_name);
  FOR_ALL_INFOS(print_info);
# undef print_info
}

# endif // Multiple_Tileras
