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


class Special_Indices {
public:
  static const oop_int_t NilObject = 0;
  static const oop_int_t FalseObject = 1;
  static const oop_int_t TrueObject = 2;
  static const oop_int_t SchedulerAssociation = 3;
  static const oop_int_t ClassBitmap = 4;
  static const oop_int_t ClassInteger = 5;
  static const oop_int_t ClassString = 6;
  static const oop_int_t ClassArray = 7;
  static const oop_int_t SmalltalkDictionary = 8; //"Do not delete!"
  static const oop_int_t ClassFloat = 9;
  static const oop_int_t ClassMethodContext = 10;
  static const oop_int_t ClassBlockContext = 11;
  static const oop_int_t ClassPoint = 12;
  static const oop_int_t ClassLargePositiveInteger = 13;
  static const oop_int_t TheDisplay = 14;
  static const oop_int_t ClassMessage = 15;
  static const oop_int_t ClassCompiledMethod = 16;
  static const oop_int_t TheLowSpaceSemaphore = 17;
  static const oop_int_t ClassSemaphore = 18;
  static const oop_int_t ClassCharacter = 19;
  static const oop_int_t SelectorDoesNotUnderstand = 20;
  static const oop_int_t SelectorCannotReturn = 21;
  static const oop_int_t ProcessSignalingLowSpace = 22;	//"was TheInputSemaphore"
  static const oop_int_t SpecialSelectors = 23;
  static const oop_int_t CharacterTable = 24;
  static const oop_int_t SelectorMustBeBoolean = 25;
  static const oop_int_t ClassByteArray = 26;
  static const oop_int_t ClassProcess = 27;
  static const oop_int_t CompactClasses = 28;
  static const oop_int_t TheTimerSemaphore = 29;
  static const oop_int_t TheInterruptSemaphore = 30;
  static const oop_int_t SelectorCannotInterpret = 34;
  static const oop_int_t MethodContextProto = 35;
# if Include_Closure_Support
  static const oop_int_t ClassBlockClosure = 36;
# endif
  static const oop_int_t BlockContextProto = 37;
  static const oop_int_t ExternalObjectsArray = 38;
  static const oop_int_t ClassPseudoContext = 39;
  static const oop_int_t ClassTranslatedMethod = 40;
  static const oop_int_t TheFinalizationSemaphore = 41;
  static const oop_int_t ClassLargeNegativeInteger = 42;

  static const oop_int_t ClassExternalAddress = 43;
  static const oop_int_t ClassExternalStructure = 44;
  static const oop_int_t ClassExternalData = 45;
  static const oop_int_t ClassExternalFunction = 46;
  static const oop_int_t ClassExternalLibrary = 47;

  static const oop_int_t SelectorAboutToReturn = 48;
  static const oop_int_t SelectorRunWithIn = 49;
# if !Include_Closure_Support
  static const oop_int_t end = 50;
# else
  static const oop_int_t SelectorAttemptToAssign = 50;
  static const oop_int_t PrimErrTableIndex = 51; // in Interpreter initializePrimitiveErrorCodes
  static const oop_int_t ClassAlien = 52;
  static const oop_int_t InvokeCallbackSelector = 53;
  static const oop_int_t ClassUnsafeAlien = 54;
  static const oop_int_t ClassWeakFinalizer = 55;
  static const oop_int_t end = 56;
# endif
};

