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

void rvm_exit() {
  Performance_Counters::print();
  OS_Interface::exit();
}

char Abstract_OS_Interface::mmap_filename[BUFSIZ] = { 0 };

void Abstract_OS_Interface::unlink_heap_file() {
  assert_always(Logical_Core::running_on_main());
  if (mmap_filename[0]) {
    lprintf("Unlinked mmap_filename: %s\n", mmap_filename);
    unlink(mmap_filename);
    mmap_filename[0] = 0;
  }
}

char* Abstract_OS_Interface::map_heap_memory(size_t total_size,
                                           size_t bytes_to_map,
                                           void*  where,
                                           off_t  offset,
                                           int    main_pid,
                                           int    flags) {
  assert_always(Max_Number_Of_Cores >= Logical_Core::group_size);
  
  assert( Memory_Semantics::cores_are_initialized() );
  assert( Using_Processes || Logical_Core::running_on_main() );
  
  
  const bool print = false;
  
  snprintf(mmap_filename, sizeof(mmap_filename), Memory_System::use_huge_pages ? "/dev/hugetlb/rvm-%d" : "/tmp/rvm-%d", main_pid);
  int open_flags = (Logical_Core::running_on_main()  ?  O_CREAT  :  0) | O_RDWR;
  
  int mmap_fd = open(mmap_filename, open_flags, 0600);
  if (mmap_fd == -1)  {
    char buf[BUFSIZ];
    snprintf(buf, sizeof(buf), "could not open mmap file, on %d, name %s, flags 0x%x",
             Logical_Core::my_rank(), mmap_filename, open_flags);
    perror(buf);
    fatal("open mmap_filename failed.");
  }
  
  if (!Memory_System::use_huge_pages && ftruncate(mmap_fd, total_size)) {
    char buf[BUFSIZ];
    snprintf(buf, sizeof(buf), "The mmap-file could not be extended to the required heap-size. Requested size was %.2f MB. ftruncate", (float) total_size / 1024.0 / 1024.0);
    perror(buf);
    unlink_heap_file();
    fatal("ftruncate");
  }
  
  assert_always(Logical_Core::running_on_main() || where != NULL);
  
  // Cannot use MAP_ANONYMOUS below because all cores need to map the same file
  void* mmap_result = map_memory(bytes_to_map, mmap_fd, flags, where, (where == NULL) ? "1st heap part" : "2nd heap part");
  
  if (mmap_result == MAP_FAILED) {
    char buf[BUFSIZ];
    snprintf(buf, sizeof(buf),
             "mmap failed on core %d. Requested %.2f MB for %s. mmap", 
             Logical_Core::my_rank(),
             (float)bytes_to_map / 1024.0 / 1024.0, 
             (where == NULL) ? "1st heap part" : "2nd heap part");
    perror(buf);
    unlink_heap_file();
    fatal("mmap");
  }

  char* mem = (char*)mmap_result;
  close(mmap_fd);
  
  if (print)
    lprintf("mmap(<requested address> 0x%x, <byte count to map> 0x%x, PROT_READ | PROT_WRITE, <flags> 0x%x, open(%s, 0x%x, 0600), <offset> 0x%x) returned 0x%x\n",
            where, bytes_to_map, flags, mmap_filename, open_flags, offset, mmap_result);
 
  assert_always( mem != NULL );
  return mem;
}

void* Abstract_OS_Interface::map_memory(size_t bytes_to_map,
                                        int    mmap_fd,
                                        int    flags,
                                        void*  start_address,
                                        const char* const usage) {
  if (Debugging)
    lprintf("mmap: About to mmap memory for %s\n", usage);
  
  if (start_address != NULL)
    flags |= MAP_FIXED;
  
  void* mmap_result = mmap(start_address, bytes_to_map, 
                           PROT_READ | PROT_WRITE,
                           flags, mmap_fd, 0);
  
  if (mmap_result == MAP_FAILED)
    return MAP_FAILED;

  if (Debugging) {
    lprintf("mmap: address requested 0x%x, result 0x%x, bytes 0x%x, flags 0x%x, offset in file 0x%x\n",
            start_address, mmap_result, bytes_to_map, flags, 0);
    lprintf("mmap: address range %p - %p\n", mmap_result, (uintptr_t)mmap_result + bytes_to_map);
  }

  if (start_address != NULL  &&  start_address != (void*)mmap_result) {
    lprintf("mmap asked for memory at 0x%x, but got it at 0x%x\n",
            start_address, mmap_result);
    return MAP_FAILED;
  }

  return mmap_result;
}

