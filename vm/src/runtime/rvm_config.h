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


// Makefile forces inclusion of this file before EVERY .c or .cpp file

// A collection of all the ifdefs we use for configuring RVM.

# ifdef __cplusplus
  # define RVM_CODE_NOT_SQUEAK_CODE // for decls already in Squeak code that we need
# endif

// for IOS

#ifdef __OBJC__
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#endif


// can invoke with -notimer or setenv SQUEAK_NOTIMER for debugging

# ifdef __APPLE__
  # define On_Apple 1
# else
  # define On_Apple 0
# endif

# if On_Apple
  # ifdef TARGET_OS_IS_FOR_IPHONE
    # define On_iOS 1
    # define On_OSX 0
  # else
    # define On_OSX 1
    # define On_iOS 0
  # endif
# endif


// This is necessary to allow large heaps on Linux
# define _FILE_OFFSET_BITS 64

// invoke template with name and default
# define DO_ALL_CONFIG_FLAGS(template) \
  template(On_Intel_Linux) \
  template(On_Tilera) \
  template(Replicate_PThread_Memory_System) /* true makes system on pthreads like Tilera, but slower */ \
  template(Max_Number_Of_Cores) \
  template(Number_Of_Channel_Buffers) \
  template(Configure_Squeak_Code_for_Tilera) \
  template(Work_Around_Extra_Words_In_Classes) \
  template(check_assertions) \
  template(check_many_assertions) \
  template(Measure) \
  template(DoBalanceChecks) \
  template(PrintFetchedContextRegisters) \
  template(Collect_Receive_Message_Statistics) \
  template(CountByteCodesAndStopAt) \
  template(CheckByteCodeTrace) \
  template(MakeByteCodeTrace) \
  template(PrintSends) \
  template(StopOnSend) \
  template(NthSendForStopping) \
  template(PrintMethodDictionaryLookups) \
  template(Multicore) \
  template(Work_Around_Barrier_Bug) \
  template(Print_Barriers) \
  template(Include_Debugging_Code) \
  template(Track_OnStackPointer) \
  template(Omit_Duplicated_OT_Overhead) \
  template(Omit_Spare_Bit) \
  template(Trace_Execution) \
  template(Trace_For_Debugging) \
  template(Track_Last_BC_For_Debugging) \
  template(Compile_Debug_Store_Checks) \
  template(Profile_Image) \
  template(Print_Scheduler) \
  template(Print_Scheduler_Verbose) \
  template(Checksum_Messages) \
  template(Always_Check_Method_Is_Correct) \
  template(Check_Prefetch) \
  template(Extra_OTE_Words_for_Debugging_Block_Context_Method_Change_Bug) \
  template(Check_Reliable_At_Most_Once_Message_Delivery) \
  template(Use_CMem) \
  template(Track_Processes) \
  template(Multiple_Tileras) \
  \
  /* The following are all used to analyze the unicore performance regression in porting to phthreads -- dmu 1/10 */ \
  template(Force_Direct_Squeak_Interpreter_Access) \
  template(Dont_Trace_Bytecode_Fetching) \
  template(Force_Direct_Timeout_Timer_List_Head_Access) \
  template(Omit_PThread_Locks) \
  template(Use_Spin_Locks) \
  template(Dont_Count_Cycles) \
  \
  template(Extra_Preheader_Word_Experiment) \
  template(Use_BufferedChannelDebug) \
  template(Use_PerSender_Message_Queue) \
  template(Include_Closure_Support) \
  template(Hammer_Safepoints) /* for debugging */ \
  \
  template(Dump_Bytecode_Cycles) \
  template(Dont_Dump_Primitive_Cycles)


# ifndef On_Intel_Linux
  # define On_Intel_Linux 0
# endif

# ifndef On_Tilera
  # define On_Tilera (!On_Apple && !On_Intel_Linux)
# endif

# ifndef Replicate_PThread_Memory_System
  # define Replicate_PThread_Memory_System 0
# endif

/* Used as the upper limit for some data structures */
# ifndef Max_Number_Of_Cores
  # define Max_Number_Of_Cores 64
# endif

# define Work_Around_Extra_Words_In_Classes On_Tilera


# ifndef Configure_Squeak_Code_for_Tilera
  # define Configure_Squeak_Code_for_Tilera 1 // Keep things consistent, even when not on Tilera
# endif

# ifndef Number_Of_Channel_Buffers
  # if On_Tilera
    # define Number_Of_Channel_Buffers 10 /*xxxxxx Seems to work, is this a good value? -- dmu 4/09?*/
  # else
    # define Number_Of_Channel_Buffers 100
  # endif
# endif

# ifndef check_assertions
  # define check_assertions 0
# endif

# ifndef check_many_assertions
  # define check_many_assertions 0
# endif

# ifndef Measure
  # define Measure 0
# endif

# ifndef Measure_Communication
  # define Measure_Communication 0
# elif Measure_Communication
  # undef Measure
  # define Measure 1
# endif


# ifndef Multicore
  # define Multicore 1
# endif


# ifndef Work_Around_Barrier_Bug
  # define Work_Around_Barrier_Bug 1
# endif

# ifndef Print_Barriers
# define Print_Barriers 0
# endif

# ifndef Include_Debugging_Code
# define Include_Debugging_Code 0
# endif

# ifndef Track_OnStackPointer
# define Track_OnStackPointer check_many_assertions
# endif

# ifndef DoBalanceChecks
#  define DoBalanceChecks check_assertions
# endif

# ifndef PrintFetchedContextRegisters
#  define PrintFetchedContextRegisters 0
# endif

// Keeping a tally of the received messages seems to have an impact on performance.
// Thus, it is only enabled when assertions are checked currently. STEFAN 2011-01-28
# ifndef Collect_Receive_Message_Statistics
# define Collect_Receive_Message_Statistics check_assertions
# endif

# ifndef CountByteCodesAndStopAt
#  define CountByteCodesAndStopAt 0
# endif

# ifndef CheckByteCodeTrace
#  define CheckByteCodeTrace 0
# endif

# ifndef MakeByteCodeTrace
#  define MakeByteCodeTrace 0
# endif

# ifndef PrintSends
#  define PrintSends 0
# endif

# ifndef StopOnSend
#  define StopOnSend ((const char *const)0)
# endif

# ifndef NthSendForStopping
#  define NthSendForStopping 0
# endif

# ifndef PrintMethodDictionaryLookups
#  define PrintMethodDictionaryLookups 0
# endif

# ifndef Omit_Duplicated_OT_Overhead
#  define Omit_Duplicated_OT_Overhead 1
# endif

# ifndef Omit_Spare_Bit
#  define Omit_Spare_Bit 1
# endif

# ifndef Checksum_Messages
#  define Checksum_Messages 0
# endif

# ifndef Always_Check_Method_Is_Correct
#  define Always_Check_Method_Is_Correct 0
# endif

// xxxxxx This flag was originall for debugging, but now I think the VM depends on it being set. -- dmu 4/09
// The Squeak VM includes an optimization to fetch a bytecode ahead to avoid memory latency.
// This flag ensures that the bytecode has been fetched ahead and stored properly.
# ifndef Check_Prefetch
#  define Check_Prefetch 1
# endif

# ifndef Use_CMem
#  define Use_CMem 0
# endif

// This flag is for debugging. -- dmu 6/10
// Track which processes are running in an uncached array
// However, primitiveRunningProcessByCore requires this to be set, and the kiviats use it.
// The primitive could be better implemented by asking each core with a message, someday. -- dmu 6/10
# ifndef Track_Processes
#  define Track_Processes 1
# endif


# ifndef Trace_Execution
#  define Trace_Execution 0 // or could be: check_assertions
# endif

# ifndef Trace_For_Debugging
#  define Trace_For_Debugging 0
# endif

# ifndef Track_Last_BC_For_Debugging
#  define Track_Last_BC_For_Debugging 0
# endif

# ifndef Compile_Debug_Store_Checks
#  define Compile_Debug_Store_Checks check_assertions
# endif

# ifndef Profile_Image
#  define Profile_Image 0
# endif

# ifndef Print_Scheduler
#  define Print_Scheduler 0
# endif

# ifndef Print_Scheduler_Verbose
#  define Print_Scheduler_Verbose 0
# endif


# ifndef Extra_OTE_Words_for_Debugging_Block_Context_Method_Change_Bug // for debugging
#  define Extra_OTE_Words_for_Debugging_Block_Context_Method_Change_Bug 0
# endif

# ifndef Check_Reliable_At_Most_Once_Message_Delivery // for debugging
#  define Check_Reliable_At_Most_Once_Message_Delivery check_assertions
# endif

# ifndef Force_Direct_Squeak_Interpreter_Access
# define Force_Direct_Squeak_Interpreter_Access 0
# endif

# ifndef Dont_Trace_Bytecode_Fetching
# define Dont_Trace_Bytecode_Fetching 0
# endif

# ifndef Force_Direct_Timeout_Timer_List_Head_Access
# define Force_Direct_Timeout_Timer_List_Head_Access 0
# endif

# ifndef Omit_PThread_Locks
# define Omit_PThread_Locks 0
# endif

# ifndef Use_Spin_Locks
# define Use_Spin_Locks 0
# endif

# ifndef Dump_Bytecode_Cycles
# define Dump_Bytecode_Cycles 0 // measuring speed, dmu, 8.2010
# endif

# ifndef Dont_Dump_Primitive_Cycles
# define Dont_Dump_Primitive_Cycles 1
# endif

# ifndef Dont_Count_Cycles
# define Dont_Count_Cycles 1 //xxxxxxx was 1, when I needed to find a deadlock -- dmu 5/10, but it slows image loading -- dmu 6/10
# endif

# if Dump_Bytecode_Cycles
# undef Dont_Count_Cycles
# define Dont_Count_Cycles 0
# endif


# ifndef Extra_Preheader_Word_Experiment
# define Extra_Preheader_Word_Experiment 0
# endif

# ifndef Use_BufferedChannelDebug
// If you turn this off, run on Mac, and run with Hammer_Safepoints with >2 cores, you can see the system break -- dmu 5/21/10
// E.g.: check_received_transmission_sequence_number: message tellCoreIAmSpinningMessage from 2 to 0 is 5730 should_be 5731
//  Fatal: message delivery error: file /Users/ungar/renaissance/rvm/src/messages/messages.cpp, line 204, function check_received_transmission_sequence_number, predicate 0, rank 0, main_rank 0, pid 95221
// STEFAN: 2011-01-14, I just tried it, and the system reliably breaks down with an assertion on sequence numbers for spin requests.
# define Use_BufferedChannelDebug 1
# endif

# ifndef Use_PerSender_Message_Queue
// This will use Shared_Memory_Queue_Per_Sender class
// It is the new default.
# define Use_PerSender_Message_Queue 1
# endif

# ifndef Include_Closure_Support
// as per: http://www.mirandabanda.org/cogblog/2008/07/22/closures-part-ii-the-bytecodes/ -- dmu 6/10
# define Include_Closure_Support 1
# endif

// Keep safepointing instead of running Smalltalk in order to find deadlock bugs
# ifndef Hammer_Safepoints
# define Hammer_Safepoints 0
# endif

# ifndef Multiple_Tileras
# define Multiple_Tileras On_Tilera
# endif




// for Squeak:

# define USE_INLINE_MEMORY_ACCESSORS
# define SQUEAK_BUILTIN_PLUGIN

# ifdef __cplusplus
extern "C" {
  # endif
void print_config();
void print_config_for_spreadsheet();
# ifdef __cplusplus
}
# endif

// To implement OT replication, I needd to add an extra argument to all
// OT-mutating operations, called esb, for enforce-store-barrier.
// To see how much overhead this caused, I have added these macros. -- dmu 9/08
# if Omit_Duplicated_OT_Overhead
#  define       DCL_ESB
#  define COMMA_DCL_ESB
#  define       USE_ESB
#  define COMMA_USE_ESB
#  define           ESB_OR_FALSE  false
#  define                  FALSE_OR_NOTHING
#  define            COMMA_FALSE_OR_NOTHING
#  define            COMMA_TRUE_OR_NOTHING
# else
#  define       DCL_ESB bool esb
#  define COMMA_DCL_ESB ,  bool esb
#  define       USE_ESB esb
#  define COMMA_USE_ESB , esb
#  define           ESB_OR_FALSE  esb
#  define                  FALSE_OR_NOTHING false
#  define            COMMA_FALSE_OR_NOTHING , false
#  define            COMMA_TRUE_OR_NOTHING  , true
# endif

// We modified Squeak's config.h but then have to change its name, so include ours here:
# if !defined(__cplusplus) && !defined(__OBJC__) && !On_iOS
# include <sqr_config.h>
# endif

