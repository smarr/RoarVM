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


Oop sample_one_core(int);

void ioProcessEvents_wrapper();

class SampleValues {
  public:
  enum {
    allCores, // 0

    runMask,
    messageNames,
    cpuCoreStats,
    allCoreStats,
    fence,

    millisecs, // 6
    cycles,
    messageStats,
    memorySystemStats,
    interpreterStats,
    objectTableStats,
    interactionStats,

    // Message_Statics:get_stats
    coreCoords, // 13
    sendTallies,
    receiveTallies,
    bufferedMessageStats,
    receiveCycles,

    // memory system
    gcStats, // 18
    heapStats,

    // interpreter
    bytecodes, // 20
    yieldCount,
    cycleCounts,
    interruptChecks,
    movedMutatedObjectStats,
    mutexStats,
    interpreterLoopStats // 26
  };
};

