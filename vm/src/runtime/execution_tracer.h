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


class Execution_Tracer: public Abstract_Tracer {
  int gc_count;
  void check_it(Oop);
  Oop ctx; // for debugging

public:
  Oop get();
  enum { k_bc, k_gc, k_proc, k_rcved_interp, k_aux } kind;
  struct bc {
    int kind;
    Oop method;
    Oop rcvr;
    int    rank    :  7;
    u_int1 is_block:  1;
    int    pc      : 12;
    int bcCount;
    int aux1;
    int aux2;
   };
  struct gc {
    int kind;
    int rank;
    int gc;
  };
  struct proc {
    int kind;
    int rank;
    Oop process;
  };
  struct rcved_interp {
    int kind;
    int to_rank;
  };
  struct aux {
    int kind;
    int rank;
    int aux1;
    int aux2;
    int id;
  };
   enum { e_kind, e_method, e_gc = e_method, e_proc = e_method, e_id = e_method, e_rcvr, e_process, e_rank, e_pc, e_is_block, e_bcCount, e_aux1, e_aux2, e_N};
  static const int max_sizes = max(sizeof(bc), max(sizeof(gc), max(sizeof(proc), max(sizeof(rcved_interp), sizeof(aux)))));

  private:
    void* entry_ptr(int i) { return (char*)buffer +  i * max_sizes; }
  public:
  void set_context(Oop c) {ctx = c;}

  Execution_Tracer(int n);
  void do_all_roots(Oop_Closure*);
  virtual void trace(Squeak_Interpreter* si);

  void set_aux(int id, const char* why) {
    aux* e = (aux*)entry_ptr(get_free_entry());
    e->kind = k_aux;
    e->id = id;
    e->rank = Logical_Core::my_rank();
    e->aux1 = (int)why;
    e->aux2 = 0;
    /*
    if (ctx.is_mem()) {
      e->aux1 = (int)ctx.as_object();
      e->aux2 = ctx.as_object()->fetchPointer(Object_Indices::InstructionPointerIndex).integerValue();
    }
    else
      e->aux1 = e->aux2 = 0;
     */
  }
  void received_current_bytecode() {
    rcved_interp* e = (rcved_interp*)entry_ptr(get_free_entry());
    e->kind = k_rcved_interp;
    e->to_rank = Logical_Core::my_rank();
  }
  void set_proc(Oop p) {
    proc* e = (proc*)entry_ptr(get_free_entry());
    e->kind = k_proc;
    e->rank = Logical_Core::my_rank();
    e->process = p;
  }
  void record_gc() {
    gc* e = (gc*)entry_ptr(get_free_entry());
    e->kind = k_gc;
    e->rank = Logical_Core::my_rank();
    e->gc = gc_count++;
  }

  void add_bc(Oop method, Oop rcvr, int pc, bool is_block, int bc_count) {
    bc* e = (bc*)entry_ptr(get_free_entry());
    e->kind = k_bc;
    e->method = method;
    e->rcvr = rcvr;
    e->pc = pc;
    e->is_block = is_block ? 1 : 0;
    e->rank = Logical_Core::my_rank();
    e->bcCount = bc_count;

    if (ctx.is_mem()) {
      e->aux1 = (int)ctx.as_object();
      e->aux2 = ctx.as_object()->fetchPointer(Object_Indices::InstructionPointerIndex).integerValue();
    }
  }

  virtual void print();
  static void print_entries(Oop, Printer*);

protected:
  Oop array_class();
  void copy_elements(int src_offset, void* dst, int dst_offset, int num_elems, Object* dst_obj);

};

