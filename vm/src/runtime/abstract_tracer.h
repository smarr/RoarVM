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


class Abstract_Tracer {
 protected:
  void* buffer;
  int size;
  int next;
  bool wrapped;
  int elem_byte_size;
  int elem_gotten_elem_size;
  OS_Interface::Mutex mutex;
 public:
  void* operator new(size_t s)   { return Memory_Semantics::shared_malloc(s); }
  void  operator delete(void* p) { Memory_Semantics::shared_free(p); }

  Abstract_Tracer(int n, int ebs, int eges)  {
    elem_byte_size = ebs;
    elem_gotten_elem_size = eges;
    buffer = Memory_Semantics::shared_malloc( n * elem_byte_size );
    size = n;
    next = 0;
    wrapped = false;
    OS_Interface::mutex_init(&mutex);
  }
  ~Abstract_Tracer() { Memory_Semantics::shared_free(buffer); }

  virtual Oop get();

 protected:
  virtual Oop array_class()  = 0;
  virtual void copy_elements(int src_offset, void* dst, int dst_offset, int num_elems, Object_p dst_obj)  = 0;
  int end_of_live_data() { return wrapped ? size : next; }


  int get_free_entry() {
     OS_Interface::abort_if_error("Tracer", OS_Interface::mutex_lock(&mutex));

    int r = next++;
    if (next < size) ;
    else if (next == size) { next = 0;  wrapped = true; }
    else fatal("should never happen");

     OS_Interface::abort_if_error("Tracer", OS_Interface::mutex_unlock(&mutex));
    return r;
  }

};

