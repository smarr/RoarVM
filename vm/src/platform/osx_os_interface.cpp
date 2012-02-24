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

# if !On_iOS
# include <CoreServices/CoreServices.h>
# include <CoreFoundation/CoreFoundation.h>
# include <IOKit/ps/IOPowerSources.h>
# include <IOKit/ps/IOPSKeys.h>

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

// Following contributed by Kristen McIntyre:

Abstract_OS_Interface::Power_Source OSX_OS_Interface::get_power_source() {
  CFTypeRef powerInfo = IOPSCopyPowerSourcesInfo();
  CFArrayRef powerSources = IOPSCopyPowerSourcesList(powerInfo);
  CFIndex count = CFArrayGetCount(powerSources);
  int ac_count = 0, battery_count = 0, offline_count = 0;
  for ( int i = 0;  i < count;  i++) {
    CFTypeRef source = (CFTypeRef)CFArrayGetValueAtIndex(powerSources, i);
    CFDictionaryRef dict = IOPSGetPowerSourceDescription(powerInfo, source);
    if (dict != NULL) {
      CFStringRef state = (CFStringRef)CFDictionaryGetValue(dict, CFSTR(kIOPSPowerSourceStateKey));
           if (CFStringCompare(state, CFSTR(kIOPSACPowerValue), 0) == kCFCompareEqualTo) ++ac_count;
      else if (CFStringCompare(state, CFSTR(kIOPSBatteryPowerValue), 0) == kCFCompareEqualTo) ++battery_count;
      else if (CFStringCompare(state, CFSTR(kIOPSOffLineValue), 0) == kCFCompareEqualTo) ++offline_count;
    }
  }
  CFRelease(powerInfo);
  CFRelease(powerSources);
  return ac_count ? AC :  battery_count ? battery : AC;
}

# else

void OSX_OS_Interface::ensure_Time_Machine_backs_up_run_directory() {}
Abstract_OS_Interface::Power_Source OSX_OS_Interface::get_power_source() { return AC; }

# endif


void OSX_OS_Interface::pin_thread_to_core(int32_t /* rank */) {
  // Mac OS X does not support setting explicit affinity to a PU
  // It only supports expressing cache affinity
  // and this is only for one process i.e. threads in a process
  // http://developer.apple.com/ReleaseNotes/Performance/RN-AffinityAPI/index.html
}

int64_t OSX_OS_Interface::get_available_main_mem_in_kb() {
  int mib[2];
  int64_t physical_memory;
  size_t length;
  
  // Get the Physical memory size
  mib[0] = CTL_HW;
  mib[1] = HW_MEMSIZE;
  length = sizeof(int64);
  sysctl(mib, 2, &physical_memory, &length, NULL, 0);

  return physical_memory / 1024;
}
