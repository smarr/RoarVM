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
# include <CoreServices/CoreServices.h>

void OSX_OS_Interface::ensure_Time_Machine_backs_up_run_directory() {
  // Since we put images in same directory as compiled rvm, tell TM to back up that directory,
  // contradicting what Xcode does. -- dmu 4/05/10
  OSStatus err;
  
  char* the_directory = getcwd(NULL, 0);
  if (the_directory == NULL) {
    lprintf("Warning: could not get Xcode to backup run directory, getenv\n");
    return;
  }
  CFURLRef dir_url_ref = CFURLCreateFromFileSystemRepresentation( kCFAllocatorDefault, 
                                                                 (const UInt8*)the_directory,
                                                                 strlen(the_directory),
                                                                 true);
  if (dir_url_ref == NULL) {
    lprintf("Warning: could not get Xcode to backup run directory, CFURLCreateFromFileSystemRepresentation\n");
    return;
  }
  
  for (   ;
       dir_url_ref != NULL;
       dir_url_ref = CFURLCreateCopyDeletingLastPathComponent(kCFAllocatorDefault, dir_url_ref) ) {
    
      char buf[BUFSIZ];
      CFURLGetFileSystemRepresentation(dir_url_ref, false, (UInt8*)buf, BUFSIZ);
      if (strcmp("/Volumes", buf) == 0)
        break;
      if ( (err = CSBackupSetItemExcluded( dir_url_ref, false, true) ) != noErr)
          printf("Warning: could not get Xcode to backup run directory, CSBackupSetItemExcluded: %s\n", buf);
      if (strcmp("/", buf) == 0)
        break;
  }

  free(the_directory);
}

