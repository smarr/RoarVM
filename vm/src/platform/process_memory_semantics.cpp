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

# if On_Tilera

Logical_Core* Process_Memory_Semantics::_my_core = NULL;
int Process_Memory_Semantics::_my_rank = -1;
u_int64 Process_Memory_Semantics::_my_rank_mask = -1;


Memory_System _memory_system;
Squeak_Interpreter _interpreter;

char Process_Memory_Semantics::mmap_filename[BUFSIZ] = { 0 };

char* Process_Memory_Semantics::map_heap_memory(off_t total_file_size,
                                          size_t bytes_to_map,
                                          size_t page_size_used_in_heap_arg,
                                          void* where,
                                          off_t offset,
                                          int main_pid,
                                          int flags) {
  assert_always(Max_Number_Of_Cores >= Logical_Core::group_size);
  
  
  
  
  const bool print = false;
  
  snprintf(mmap_filename, sizeof(mmap_filename), Memory_System::use_huge_pages ? "/dev/hugetlb/rvm-%d" : "/tmp/%d", main_pid);
  int open_flags = (where == NULL  ?  O_CREAT  :  0) | O_RDWR;
  int mmap_fd = open(mmap_filename, open_flags, 0600);
  if (mmap_fd == -1)  {
    char buf[BUFSIZ];
    sprintf(buf, "could not open mmap file, on %d, name %s, flags 0x%x",
            Logical_Core::my_rank(), mmap_filename, open_flags);
    perror(buf);
  }
  
  if (!Memory_System::use_huge_pages && ftruncate(mmap_fd, total_file_size)) {
    perror("ftruncate");
    fatal("ftruncate");
  }
  
  // Cannot use MAP_ANONYMOUS below because all cores need to map the same file
  int32 mmap_result = (int32)mmap(where, bytes_to_map, PROT_READ | PROT_WRITE,  flags, mmap_fd, offset);
  if (check_many_assertions)
    lprintf("mmapp: address requested 0x%x, result 0x%x, bytes 0x%x, flags 0x%x, offset in file 0x%x\n",
            where, mmap_result, bytes_to_map, flags, offset);
  if (print)
    lprintf("mmap(<requested address> 0x%x, <byte count to map> 0x%x, PROT_READ | PROT_WRITE, <flags> 0x%x, open(%s, 0x%x, 0600), <offset> 0x%x) returned 0x%x\n",
            where, bytes_to_map, flags, mmap_filename, open_flags, offset, mmap_result);
  if (mmap_result == -1) {
    char buf[BUFSIZ];
    snprintf(buf, sizeof(buf),"mmap failed on tile %d", Logical_Core::my_rank());
    perror(buf);
    fatal("mmap");
  }
  if (where != NULL  &&  where != (void*)mmap_result) {
    lprintf("mmap asked for memory at 0x%x, but got it at 0x%x\n",
            where, mmap_result);
    fatal("mmap was uncooperative");
  }
  char* mem = (char*)mmap_result;
  close(mmap_fd);
  
  assert_always( mem != NULL );
  return mem;
}

# endif

