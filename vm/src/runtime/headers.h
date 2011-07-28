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


/**
 * Naming conventions
 *
 *  files   - names are lower-case, words are separated by underscores
 *          - names of Squeak-related files like RVMPlugin keep their
 *            Squeak-like naming scheme
 *
 *  classes - words start with a capital letter
 *          - words are separated by an underscore
 *          - abbreviations are completely capitalized
 *          - Oop isn't an abbreviation of object-oriented pointer ;)
 */

# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <stdarg.h>
# include <ctype.h>
# include <sys/mman.h>
# include <fcntl.h>
# include <unistd.h>
# include <signal.h>
# include <errno.h>

# if On_Tilera
#  include <tmc/mem.h>
#  include <asm/page.h>
#  include <arch/sim.h>
#  include <ilib.h>
#  include <arch/cycle.h>
#  include <arch/udn.h> 
#  if Multiple_Tileras
#    include <tilepci.h>
#  endif

#  if Use_CMem
#   include <tmc/cmem.h>
#  endif
# else
#  include "synced_queue.h"
#  include "buffered_channel.h"
#  include "buffered_channel_debug.h"
# endif

# if On_iOS
#  include <libkern/OSAtomic.h>
# elif On_Apple
#  include <monitor.h>
#  include <libkern/OSAtomic.h>
# else
#  include <sys/resource.h>
# endif


// # include "rvm_config.h" // makefile (or Prefix Header setting in Xcode language settings) includes this before EVERY .c or .cpp or .m file

# include "my_rank.h"

# include "tags.h"
# include "error_handling.h"
# include "printer.h"
# include "types.h"
# include "utils.h"

# include "os_interface.h"


# if !On_iOS
# include "host_pci_info.h"

# include "abstract_zero_copy_command_queue_endpoint.h"

# include "chip_to_chip_direct_to_hypervisor_zero_copy_endpoint.h"
# include "chip_to_chip_direct_to_hypervisor_zero_copy_sender.h"
# include "chip_to_chip_direct_to_hypervisor_zero_copy_receiver.h"

# include "chip_to_chip_zero_copy_command_queue_endpoint.h"
# include "chip_to_chip_zero_copy_command_sender.h"
# include "chip_to_chip_zero_copy_command_receiver.h"

# include "tilera_chip_to_chip_message_queue.h"
# endif


# include "message_queue.h"
# include "cpu_coordinate.h"

# include "abstract_os_interface.h"
# include "ilib_os_interface.h"
# include "posix_os_interface.h"
# include "osx_os_interface.h"

# include "performance_counters.h"

# include "safepoint_ability.h"
# include "safepoint_request_queue.h"

# include "rank_set.h"

# include "measurements.h"

# include "rvm_bitmap.h"
# include "bytemap.h"

# include "preheader.h"


# include "tracked_ptr_registry.h"
# include "tracked_ptr.h"

# include "object_p.h"

# include "abstract_oop.h"
# include "oop.h"
# include "oop_closure.h"

# include "abstract_cpu_coordinate.h"
# include "dummy_cpu_coordinate.h"
# include "tile_cpu_coordinate.h"

# include "receive_marker.h"
# include "message_templates.h"
# include "message_statics.h"
# include "abstract_message.h"

# include "abstract_message_queue.h"
# include "shared_memory_message_queue.h"
# include "shared_memory_message_queue_per_sender.h"
# include "ilib_message_queue.h"

# include "memory_semantics.h"
# include "abstract_memory_semantics.h"
# include "thread_memory_semantics.h"
# include "process_memory_semantics.h"

# include "logical_core.h"

# include "interpreter_subset_for_control_transfer.h"
# include "message_stats.h"
# include "message_classes.h"

# include "message_or_ack_request.h"
# include "deferred_request.h"
# include "interactions.h"

# include "timeout_timer.h"
# include "timeout_deferral.h"

# include "special_indices.h"
# include "roots.h"
# include "object_indices.h"

# include "abstract_mutex.h"
# include "safepoint.h"
# include "scheduler_mutex.h"
# include "semaphore_mutex.h"

# include "abstract_object_heap.h"
# include "multicore_object_heap.h"

# include "header_type.h"
# include "word_containing_object_type.h"
# include "chunk.h"
# include "object.h"
# include "process_field_locator.h"

# include  "abstract_tracer.h"
# include      "core_tracer.h"
# include       "oop_tracer.h"
# include "execution_tracer.h"
# include "profiling_tracer.h"
# include "gc_debugging_tracer.h"



# include "object_table.h"
# include "abstract_object_table.h"
# include "segmented_object_table.h"
# include "multicore_object_table.h"
# include "dummy_object_table.h"

# include "memory_system.h"

# include "runtime_tester.h"

# include "method_cache.h"
# include "at_cache.h"

# include "externals.h"
# include "abstract_primitive_table.h"
# include "primitive_table.h"
# include "obsolete_indexed_primitive_table.h"
# include "obsolete_named_primitive_table.h"
# include "external_primitive_table.h"
# include "debug_store_checks.h"
# include "squeak_interpreter.h"

# include "squeak_image_reader.h"


# include "gc_oop_stack.h"
# include "abstract_mark_sweep_collector.h"
# include "indirect_oop_mark_sweep_collector.h"
# include "mark_sweep_collector.h"

# include "RVMPlugin.h"

# include "oop.inline.h"
# include "chunk.inline.h"
# include "object.inline.h"
# include "abstract_object_heap.inline.h"
# include "multicore_object_heap.inline.h"
# include "multicore_object_table.inline.h"
# include "segmented_object_table.inline.h"
# include "memory_system.inline.h"

# include "debug_helper.h"

