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


static const int evtBuf_size = 8;

// TODO: the class macros should be defined where they are actually used, move to correct file


// CAVEAT: if a messsage contains an Oop, it will get handled upon reception (unless the receiver holds a safepoint).
// But if the receiver is spinning in a safepoint, the holder may be doing a GC. If no action has been taken to preserve the Oop,
// it can get reclaimed by the collector. For instance, sampleOneCoreResponse was getting its array smashed.

// Furthermore, the current Mac Message_Queue is not searched for roots during a GC; enqueued messages could lose roots.
// Perhaps the safest course would be for the sender to register the oop, and wait for an ack.

// -- dmu 10/1/10

# define FOR_ALL_MESSAGES_DO(template) \
template(noMessage,abstractMessage, (), (), , , no_ack, dont_delay_when_have_acquired_safepoint) \
template(ackMessage,abstractMessage, (Message_Statics::messages m), (), { header = Message_Statics::encode_msg_type_for_ack(m); /*HACK*/ }, , no_ack, dont_delay_when_have_acquired_safepoint) \
template(addedScheduledProcessMessage,abstractMessage, (), (), , , no_ack, dont_delay_when_have_acquired_safepoint) \
\
template(aboutToWriteReadMostlyMemoryMessage,abstractMessage, (void* p, int n), (), {addr = p; nbytes = n;}, void* addr; int nbytes;, no_ack, dont_delay_when_have_acquired_safepoint) \
template(addObjectFromSnapshotMessage,abstractMessage, (Oop d, Object* s), (), {dst_oop = d; src_obj_wo_preheader = s;}, Oop dst_oop; Object* src_obj_wo_preheader; void do_all_roots(Oop_Closure*);, no_ack, dont_delay_when_have_acquired_safepoint) \
template(addObjectFromSnapshotResponse,abstractMessage, (Object* d), (), {dst_obj = d;}, Object* dst_obj; void do_all_roots(Oop_Closure*);, no_ack, dont_delay_when_have_acquired_safepoint) \
template(broadcastInterpreterDatumMessage,abstractMessage, (int s, int o, u_int64 d), (), {datum_size = s; datum_byte_offset = o; datum = d;}, int datum_size; int datum_byte_offset; u_int64 datum;, no_ack, dont_delay_when_have_acquired_safepoint) /*xxxxxx simple if no wait*/\
template(doAllRootsHereMessage,abstractMessage, (Oop_Closure* oc, bool igp), (), { closure = oc; is_gc_permitted = igp; }, Oop_Closure* closure; bool is_gc_permitted; , no_ack, dont_delay_when_have_acquired_safepoint) \
\
template(getNextEventMessage,abstractMessage, (), (), , , no_ack, dont_delay_when_have_acquired_safepoint) \
template(getNextEventResponse,abstractMessage, (), () , {  for (int i = 0;  i < evtBuf_size;  ++i) evtBuf[i] = 0; }, int evtBuf[evtBuf_size]; bool got_one; , no_ack, dont_delay_when_have_acquired_safepoint) \
\
template(selfDestructMessage,abstractMessage, (const char* w), (), {why = w;}, const char* why; , no_ack, dont_delay_when_have_acquired_safepoint) \
template(selfQuitMessage,abstractMessage, (const char* w), (), {why = w;}, const char* why; , no_ack, dont_delay_when_have_acquired_safepoint) \
\
template(enforceCoherenceAfterEachCoreHasStoredIntoItsOwnHeapMessage,abstractMessage, (), (), , , post_ack_for_correctness, dont_delay_when_have_acquired_safepoint) \
template(enforceCoherenceBeforeEachCoreStoresIntoItsOwnHeapMessage,abstractMessage, (), (), , , post_ack_for_correctness, dont_delay_when_have_acquired_safepoint) \
template(enforceCoherenceBeforeSenderStoresIntoAllHeapsMessage,abstractMessage, (), (), , , post_ack_for_correctness, dont_delay_when_have_acquired_safepoint) \
template(flushFreeContextsMessage,abstractMessage, (), (), , , post_ack_for_correctness, dont_delay_when_have_acquired_safepoint) /* xxxxxx could be simple if no waiting */\
\
template(flushInterpreterCachesMessage,abstractMessage, (), (), , , no_ack, dont_delay_when_have_acquired_safepoint) \
template(flushMethodCacheMessage,abstractMessage, (), (), , , no_ack, dont_delay_when_have_acquired_safepoint) \
template(flushSelectiveMessage,abstractMessage, (Oop s), (), { selector = s; }, Oop selector; void do_all_roots(Oop_Closure*); , no_ack, dont_delay_when_have_acquired_safepoint) \
template(flushByMethodMessage,abstractMessage, (Oop x), (), { method = x; }, Oop method; void do_all_roots(Oop_Closure*); , no_ack, dont_delay_when_have_acquired_safepoint) \
\
template(setExtraWordSelectorMessage,abstractMessage, (Oop s), (), { selector = s; }, Oop selector; , no_ack, dont_delay_when_have_acquired_safepoint) \
template(setEmergencySemaphoreMessage,abstractMessage, (Oop s), (), { semaphore = s; }, Oop semaphore; , no_ack, dont_delay_when_have_acquired_safepoint) \
\
template(hereIsARootResponse,abstractMessage, (Oop r, Oop* a, Object* c, Oop_Closure* cl), (), { root = r; addr = a; container_or_null = c; closure = cl;}, Oop root; Oop* addr; Object* container_or_null; Oop_Closure* closure; void do_all_roots(Oop_Closure*);, no_ack, dont_delay_when_have_acquired_safepoint) \
\
template(imageNamePutMessage,abstractMessage, (char* b, unsigned int n), (), { image_name = b; len = n; }, char* image_name; unsigned int len;, post_ack_for_correctness, dont_delay_when_have_acquired_safepoint) /* xxxxxx simple if no wait */ \
\
template(loadFunctionFromPluginMessage,abstractMessage, (const char* f, const char* p), (), {fn_name = f; plugin_name = p;}, const char* fn_name; const char* plugin_name; , no_ack, dont_delay_when_have_acquired_safepoint) \
\
template(loadFunctionFromPluginResponse,abstractMessage, (fn_t f), (), { fn = f;}, fn_t fn; , no_ack, dont_delay_when_have_acquired_safepoint) \
template(newValueForOopMessage,abstractMessage, (Oop x, Oop*p), (), {addr = p; newValue = x;}, Oop* addr; Oop newValue; void do_all_roots(Oop_Closure*);, no_ack, dont_delay_when_have_acquired_safepoint) \
\
template(noMoreRootsResponse,abstractMessage, (), (), , , no_ack, dont_delay_when_have_acquired_safepoint)  \
template(postGCActionMessage,abstractMessage, (bool f, bool is_a), (), {fullGC = f; sender_is_able_to_safepoint = is_a; }, bool fullGC; bool sender_is_able_to_safepoint; , no_ack, dont_delay_when_have_acquired_safepoint) \
template(preGCActionMessage,abstractMessage, (bool f), (), {fullGC = f;}, bool fullGC;, post_ack_for_correctness, dont_delay_when_have_acquired_safepoint) \
template(recycleContextIfPossibleMessage,abstractMessage, (Oop c), (), {ctx = c;}, Oop ctx; void do_all_roots(Oop_Closure*); , no_ack, dont_delay_when_have_acquired_safepoint) \
\
template(screenInfoMessage,abstractMessage, (), (), , , no_ack, dont_delay_when_have_acquired_safepoint) \
template(screenInfoResponse,abstractMessage, (int sz, int full), (), { screenSize = sz; fullScreenFlag = full; }, int screenSize; int fullScreenFlag; , no_ack, dont_delay_when_have_acquired_safepoint) \
\
template(requestSafepointOnOtherCoresMessage,abstractMessage, (const char* w), (), { why = w; }, const char* why; , no_ack, dont_delay_when_have_acquired_safepoint) \
\
template(grantSafepointMessage,abstractMessage, (int sn), (), { sequence_number = sn; }, int sequence_number;, no_ack, dont_delay_when_have_acquired_safepoint) \
\
template(releaseOtherCoresFromSafepointMessage,abstractMessage, (), (), , , no_ack, dont_delay_when_have_acquired_safepoint) \
\
template(requestCoreToSpinMessage,abstractMessage, (int fw, int sn, const char* w), (), { for_whom = fw; sequence_number = sn; why = w; }, int for_whom;  int sequence_number; const char* why;, no_ack, dont_delay_when_have_acquired_safepoint)\
\
template(tellCoreToStopSpinningMessage,abstractMessage, (int sn), (), {sequence_number = sn;}, int sequence_number;, no_ack, dont_delay_when_have_acquired_safepoint)\
\
template(tellCoreIAmSpinningMessage,abstractMessage, (int sn, bool was), (), {sequence_number = sn; was_spinning = was;}, int sequence_number; bool was_spinning; , no_ack, dont_delay_when_have_acquired_safepoint) \
template(sampleOneCoreMessage,abstractMessage, (int w), (), {what_to_sample = w;}, int what_to_sample;, no_ack, delay_when_have_acquired_safepoint) \
template(sampleOneCoreResponse,abstractMessage, (Oop r), (), {result = r;}, Oop result;  void do_all_roots(Oop_Closure*);, post_ack_for_correctness, dont_delay_when_have_acquired_safepoint) \
template(scanCompactOrMakeFreeObjectsMessage,abstractMessage, (bool c, Abstract_Mark_Sweep_Collector* g), (), {compacting = c; gc_or_null = g;}, bool compacting; Abstract_Mark_Sweep_Collector* gc_or_null; , post_ack_for_correctness, dont_delay_when_have_acquired_safepoint) \
template(startInterpretingMessage,abstractMessage, (), (), , , no_ack, dont_delay_when_have_acquired_safepoint) \
template(verifyInterpreterAndHeapMessage,abstractMessage, (), (), , , post_ack_for_correctness, dont_delay_when_have_acquired_safepoint) \
template(zapUnusedPortionOfHeapMessage,abstractMessage, (), (), , , post_ack_for_correctness, dont_delay_when_have_acquired_safepoint) \
\
/* have to do updateWholeInterpreter the hard way because of header order, sigh */ \
template(distributeInitialInterpreterMessage,abstractMessage, (Squeak_Interpreter* i), (), { interp = i; }, Squeak_Interpreter* interp;, post_ack_for_correctness, dont_delay_when_have_acquired_safepoint) \
template(updateEnoughInterpreterToTransferControlMessage,abstractMessage, (), (), , Interpreter_Subset_For_Control_Transfer subset; void send_to(int); void do_all_roots(Oop_Closure*);, no_ack, dont_delay_when_have_acquired_safepoint) \
template(transferControlMessage,updateEnoughInterpreterToTransferControlMessage, (), (), , , no_ack, dont_delay_when_have_acquired_safepoint) \
template(runPrimitiveMessage,updateEnoughInterpreterToTransferControlMessage, (int c, fn_t f), (), { argCount = c; fn = f; }, int argCount; fn_t fn; , no_ack, delay_when_have_acquired_safepoint) \
template(runPrimitiveResponse, updateEnoughInterpreterToTransferControlMessage, (), (), , , no_ack, dont_delay_when_have_acquired_safepoint) \
\

// TODO: the following macros do not belong here, move them to a better place

# define WAIT_FOR_MESSAGE(msg_type_to_receive, src_rank)  \
  msg_type_to_receive##_class(&The_Receive_Marker).receive_and_handle_messages_returning_a_match(src_rank)

# define SEND_THEN_WAIT_FOR_MESSAGE(msg_to_send, dst, msg_type_to_receive) \
  (msg_to_send).send_then_receive_and_handle_messages_returning_a_match(dst, msg_type_to_receive##_class(&The_Receive_Marker))

# define SEND_THEN_WAIT_AND_RETURN_MESSAGE(msg_to_send, dst, msg_type_to_receive, var_name) \
  msg_type_to_receive##_class var_name(&The_Receive_Marker); \
  (msg_to_send).send_then_receive_and_handle_messages_returning_a_match(dst, var_name)

