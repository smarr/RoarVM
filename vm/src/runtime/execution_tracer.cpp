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


Execution_Tracer::Execution_Tracer(int n) : Abstract_Tracer(n, max_sizes, e_N) {
  gc_count = 0;
  ctx = Oop::from_int(0);
}


Oop Execution_Tracer::array_class() { return The_Squeak_Interpreter()->splObj(Special_Indices::ClassArray); }

void Execution_Tracer::do_all_roots(Oop_Closure* oc) {
  oc->value(&ctx, NULL);
  for (int i = 0;  i < end_of_live_data();  ++i) {
    bc* bcp = (bc*)entry_ptr(i);  proc* procp = (proc*)bcp;
    switch (bcp->kind) {
        default: fatal(); break;
        case k_bc:
          oc->value(&bcp->method, NULL);
          oc->value(&bcp->rcvr, NULL);
          break;
        case k_gc:
        case k_rcved_interp:
        case k_aux:
          break;
        case k_proc:
          oc->value(&procp->process, NULL);
          break;
    }
  }
}


void Execution_Tracer::trace(Squeak_Interpreter* si) {
  int pc = si->localIP() - si->method_obj()->as_u_char_p();
  add_bc(si->method(), si->roots.receiver, pc, si->activeContext_obj()->is_this_context_a_block_context(), si->bcCount);
}


void Execution_Tracer::print() {
  lprintf( "printing history\n");
  The_Squeak_Interpreter()->print_all_stack_traces(dittoing_stdout_printer);
  lprintf( "\n\n\nprinting last bytecodes\n");
  Oop entries = get();
  check_it(entries);

  print_entries(entries, debug_printer);
}

Oop Execution_Tracer::get() {
  Oop r = Abstract_Tracer::get();
  check_it(r);
  return r;
}


void Execution_Tracer::copy_elements(int src_offset, void* dst, int dst_offset, int num_elems, Object* dst_obj) {
  lprintf( "copy_elements src_offset %d, buffer 0x%x, dst 0x%x, dst_offset %d, num_elems %d, dst_obj 0x%x, next %d\n",
          src_offset, buffer, dst, dst_offset, num_elems, dst_obj, next);



  Oop* dst_oop = (Oop*)dst  +  dst_offset * e_N;
  // zero-fill for GC sake
  for (int i = 0;  i < num_elems;  ++i)
    for (int j = 0;  j < e_N;  ++j)
      dst_oop[i * e_N  +  j] = Oop::from_int(0);

  for (int i = 0;  i < num_elems;  ++i, dst_oop += e_N) {
    bc* bcp = (bc*)entry_ptr(i);  gc* gcp = (gc*)bcp;  proc* procp = (proc*)bcp; rcved_interp* rip = (rcved_interp*)bcp; aux* auxp = (aux*)bcp;
    dst_oop[e_kind] = Oop::from_int(bcp->kind);
    switch (bcp->kind) {
      default: fatal(); break;
      case k_aux: {
        dst_oop[e_id      ] = Oop::from_int(auxp->id);
        dst_oop[e_aux1    ] = Oop::from_int(auxp->aux1);
        dst_oop[e_aux2    ] = Oop::from_int(auxp->aux2);
        dst_oop[e_rank    ] = Oop::from_int(auxp->rank);
      }
        break;
      case k_bc: {
        dst_oop[e_method  ] = bcp->method;  Object* mo = dst_oop[e_method].as_object();
        dst_oop[e_rcvr    ] = bcp->rcvr;

        dst_oop[e_rank    ] = Oop::from_int(bcp->rank);
        dst_oop[e_bcCount ] = Oop::from_int(bcp->bcCount);
        dst_oop[e_pc      ] = Oop::from_int(bcp->pc  -  (mo->first_byte_address() - mo->as_char_p()));
        dst_oop[e_aux1    ] = Oop::from_int(bcp->aux1);
        dst_oop[e_aux2    ] = Oop::from_int(bcp->aux2);
        dst_oop[e_is_block] = bcp->is_block ? The_Squeak_Interpreter()->roots.trueObj : The_Squeak_Interpreter()->roots.falseObj;

        assert_always(dst_oop[e_method ].is_mem());
        assert_always(dst_oop[e_pc  ].is_int());
        assert_always(dst_oop[e_rank].is_int());
        assert_always(dst_oop[e_is_block] == The_Squeak_Interpreter()->roots.trueObj  ||  dst_oop[e_is_block] == The_Squeak_Interpreter()->roots.falseObj);
        assert_always( (int(dst_oop) - int(dst_obj) - Object::BaseHeaderSize) % e_N  == 0 );
      }
        break;
      case k_gc:
        dst_oop[e_gc  ] = Oop::from_int(gcp->gc);
        dst_oop[e_rank] = Oop::from_int(gcp->rank);
        break;
      case k_proc:
        dst_oop[e_rank]     = Oop::from_int(procp->rank);
        dst_oop[e_process ] = procp->process;
        break;
      case k_rcved_interp:
        dst_oop[e_rank] = Oop::from_int(rip->to_rank);
        break;
    }
  }
  dst_obj->my_heap()->check_multiple_stores_for_generations_only(dst_oop, num_elems * e_N);
}



void Execution_Tracer::check_it(Oop ents) {
  Object* eo = ents.as_object();
  int wl = eo->fetchWordLength();
  int n = wl / e_N;

  for (int i = 0;  i < n;  ++i) {
    int x = -1;
    Oop rank = eo->fetchPointer(x   =   i * e_N  +  e_rank);
    assert_always( rank.is_int());
    switch (eo->fetchPointer(i * e_N  +  e_kind).integerValue()) {
        default: fatal();
      case k_bc: {
          Oop meth         = eo->fetchPointer(x   =   i * e_N  +  e_method); assert_always(meth.is_mem());
          Oop rcvr         = eo->fetchPointer(x   =   i * e_N  +  e_rcvr);
          Oop pc           = eo->fetchPointer(x   =   i * e_N  +  e_pc  );  assert_always(pc.is_int());
          Oop is_block     = eo->fetchPointer(x   =   i * e_N  +  e_is_block); assert_always(is_block == The_Squeak_Interpreter()->roots.trueObj || is_block == The_Squeak_Interpreter()->roots.falseObj);
          break;
      }
        case k_gc: {
          Oop gc           = eo->fetchPointer(x   =   i * e_N  +  e_gc      ); assert_always(gc.is_int());
        }
          break;
        case k_proc: {
          Oop process      = eo->fetchPointer(x   =   i * e_N  +  e_process); assert_always(process.is_mem());
        }
          break;
        case k_rcved_interp:
          break;
        case k_aux: break;
    }
  }
}

void Execution_Tracer::print_entries(Oop ents, Printer* p) {
  Object* eo = ents.as_object();
  int n = eo->fetchWordLength()  /  e_N;
  int x;

  for (int i = 0;  i < n;  ++i) {
    int rank = eo->fetchPointer(x   =   i * e_N  +  e_rank).integerValue();
    p->printf("%3d on %2d: ", i - n, rank);

    switch (eo->fetchPointer(i * e_N  +  e_kind).integerValue()) {
        default: fatal(); break;
        case k_proc: {
          Oop process      = eo->fetchPointer(i * e_N  +  e_process); assert(process.is_mem());
          p->printf("switch to process 0x%x", process.as_object());
        }
          break;

      case k_aux: {
        int rank         = eo->fetchPointer(i * e_N  +  e_rank).integerValue();
        int aux1         = eo->fetchPointer(i * e_N  +  e_aux1).integerValue();
        int aux2         = eo->fetchPointer(i * e_N  +  e_aux2).integerValue();
        int id           = eo->fetchPointer(i * e_N  +  e_id  ).integerValue();
        // p->printf("on %d: aux1 0x%x aux2 0x%x id %d", rank, aux1, aux2, id);
        p->printf("on %d: aux1 %s id %d", rank, aux1, id);
      }
        break;

        case k_gc:
          p->printf("gc: %d",  eo->fetchPointer(i * e_N  + e_gc).integerValue());
          break;

        case k_rcved_interp:
          p->printf("received interp");
          break;

        case k_bc: {
          Oop meth         = eo->fetchPointer(i * e_N  +  e_method); assert(meth.is_mem());
          Oop rcvr         = eo->fetchPointer(i * e_N  +  e_rcvr);
          int pc           = eo->fetchPointer(i * e_N  +  e_pc  ).integerValue();
          bool is_block    = eo->fetchPointer(i * e_N  +  e_is_block) == The_Squeak_Interpreter()->roots.trueObj;
          int aux1         = eo->fetchPointer(i * e_N  +  e_aux1).integerValue();
          int aux2         = eo->fetchPointer(i * e_N  +  e_aux2).integerValue();

          Oop klass = rcvr.fetchClass();
          Oop sel, mclass;
          bool have_sel_and_mclass = klass.as_object()->selector_and_class_of_method_in_me_or_ancestors(meth, &sel, &mclass);
          if (!have_sel_and_mclass) continue;
          if (mclass == The_Squeak_Interpreter()->roots.nilObj)
            mclass = rcvr.fetchClass();

          p->printf("rcvr: ");  rcvr.print(p);
          if (mclass != klass) {
            p->printf("(");
            bool is_meta;
            Oop mclassName = mclass.as_object()->name_of_class_or_metaclass(&is_meta);
            if (is_meta) p->printf("class ");
            mclassName.as_object()->print_bytes(p);
            p->printf(")");
          }
          p->printf("  >>  ");
          sel.as_object()->print_bytes(p);
          p->printf(is_block ? " [] " : "    ");
          p->printf(", pc: %d, ", pc);

          Object* mo = meth.as_object();
          u_char bc = mo->first_byte_address()[pc];
          The_Squeak_Interpreter()->printBC(bc, p);

          p->printf(", bcCount %d", eo->fetchPointer(i * e_N  +  e_bcCount).integerValue());
          if (aux1)  p->printf(", aux1 = 0x%x", aux1);
          if (aux1)  p->printf(", aux1 = 0x%x", aux2);


        }
        break;
    }
    p->nl();
  }
}

