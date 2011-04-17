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


class Object_Indices {
public:
  // Association indices
  static const int ValueIndex = 1;

  static const int CharacterValueIndex = 0;

  // CharacterScanner
  static const int CrossedX = 258;
  static const int EndOfRun = 257;

  // Class indices
  // "Class Class"
  static const int SuperclassIndex = 0;
  static const int MessageDictionaryIndex = 1;
  static const int InstanceSpecificationIndex = 2;
  static const int Class_Name_Index = 6; // not in Squeak VM
  static const int This_Class_Index = 5;
  // "Fields of a message dictionary"
  static const int MethodArrayIndex = 1;
  static const int SelectorStart = 2;

  // Context indices
  // "Class MethodContext"
  static const int SenderIndex = 0;
  static const int InstructionPointerIndex = 1;
  static const int StackPointerIndex = 2;
  static const int MethodIndex = 3;
# if Include_Closure_Support
  static const int ClosureIndex = 4; // called receiverMap in image
# endif
  static const int ReceiverIndex = 5;
  static const int CtextTempFrameStart = 6; // copy of TempFrameStart in interp
  static const int ContextFixedSizePlusHeader = CtextTempFrameStart + 1;
  static const int SmallContextSize = (ContextFixedSizePlusHeader + 16) * bytesPerWord; // 16 indexable fields

  // "Large contexts have 56 indexable fileds.  Max with single header word."
  // "However note that in 64 bits, for now, large contexts have 3-word headers"
  static const int LargeContextSize = (ContextFixedSizePlusHeader + 56) * bytesPerWord;



  static const int TempFrameStart = CtextTempFrameStart;
  // "Class BlockContext"
  static const int CallerIndex = 0;
  static const int Free_Chain_Index = 0;
  static const int BlockArgumentCountIndex = 3;
  static const int InitialIPIndex = 4;
  static const int HomeIndex = 5;
  
  // "Class BlockClosure"
  static const int BlockMethodIndex = 0;
  
# if Include_Closure_Support
  static const int ClosureOuterContextIndex = 0;
  static const int ClosureStartPCIndex = 1;
  static const int ClosureNumArgsIndex = 2;
  static const int ClosureFirstCopiedValueIndex = 3;
  static const int ClosureWordCount = 4;
# endif

  // directory lookup
  static const int DirEntryFound = 0;
  static const int DirNoMoreEntries = 1;
  static const int DirBadPath = 2;

  // Message indices
  static const int MessageSelectorIndex = 0;
  static const int MessageArgumentsIndex = 1;
  static const int MessageLookupClassIndex = 2;

  // CompiledMethod indices
  static const int HeaderIndex = 0;
  static const int LiteralStart = 1;
  static const int LiteralCountShift = 10;
  static const int LiteralCountMask = 0xff;
  static const int ArgumentCountShift = 25;
  static const int ArgumentCountMask = 0xf;
  static const int TemporaryCountShift = 19;
  static const int TemporaryCountMask = 0x3f;
  static const int Primitive_Index_Low_Shift  =      Tag_Size;
  static const int Primitive_Index_High_Shift = 19 + Tag_Size;
  static const int Primitive_Index_Low_Mask =       0x1FF << Tag_Size;
  static const int Primitive_Index_High_Mask = 0x10000000 << Tag_Size;

  static const int External_Primitive_Literal_Index = 0;

  // External_Primitive_Literal indices
  static const int EPL_Module_Name = 0;
  static const int EPL_Function_Name = 1;
  static const int EPL_Session_ID = 2;
  static const int EPL_External_Primitive_Table_Index = 3;
  static const int EPL_Length = 4;

  // Point indices
  static const int XIndex = 0;
  static const int YIndex = 1;


  /*
   static const Oop ConstMinusOne = Oop::from_int(-1);
   static const Oop ConstZero = Oop::from_int(0);
   static const Oop ConstOne = Oop::from_int(1);
   static const Oop ConstTwo = Oop::from_int(2);
   */

  // Stream
  static const int StreamArrayIndex = 0;
  static const int StreamIndexIndex = 1;
  static const int StreamReadLimitIndex = 2;
  static const int StreamWriteLimitIndex = 3;

  // Scheduler
  // "Class ProcessorScheduler"
  static const int ProcessListsIndex = 0;
  static const int ActiveProcessIndex = 1;
  // "Class LinkedList"
  static const int FirstLinkIndex = 0;
  static const int LastLinkIndex = 1;
  // "Class Semaphore"
  static const int ExcessSignalsIndex = 2;
  // "Class Link"
  static const int NextLinkIndex = 0;
  // "Class Process"
  static const int SuspendedContextIndex = 1;
  static const int PriorityIndex = 2;
  static const int MyListIndex = 3;
  static const int ProcessName = 5;


};

