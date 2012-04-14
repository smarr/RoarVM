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

Primitive_Table primitiveTable;


Primitive_Table::Primitive_Table() : Abstract_Primitive_Table(576, false) {
  init_here(0, primitiveFail);
  init_here(1, primitiveAdd);
  init_here(2, primitiveSubtract);
  init_here(3, primitiveLessThan);
  init_here(4, primitiveGreaterThan);
  init_here(5, primitiveLessOrEqual);
  init_here(6, primitiveGreaterOrEqual);
  init_here(7, primitiveEqual);
  init_here(8, primitiveNotEqual);
  init_here(9, primitiveMultiply);
  init_here(10, primitiveDivide);
  init_here(11, primitiveMod);
  init_here(12, primitiveDiv);
  init_here(13, primitiveQuo);
  init_here(14, primitiveBitAnd);
  init_here(15, primitiveBitOr);
  init_here(16, primitiveBitXor);
  init_here(17, primitiveBitShift);
  init_here(18, primitiveMakePoint);
  init_here(19, primitiveFail);
  init_here(20, 39, primitiveFail);
  init_here(40, primitiveAsFloat);
  init_here(41, primitiveFloatAdd);
  init_here(42, primitiveFloatSubtract);
  init_here(43, primitiveFloatLessThan);
  init_here(44, primitiveFloatGreaterThan);
  init_here(45, primitiveFloatLessOrEqual);
  init_here(46, primitiveFloatGreaterOrEqual);
  init_here(47, primitiveFloatEqual);
  init_here(48, primitiveFloatNotEqual);
  init_here(49, primitiveFloatMultiply);
  init_here(50, primitiveFloatDivide);
  init_here(51, primitiveTruncated);
  init_here(52, primitiveFractionalPart);
  init_here(53, primitiveExponent);
  init_here(54, primitiveTimesTwoPower);
  init_here(55, primitiveSquareRoot);
  init_here(56, primitiveSine);
  init_here(57, primitiveArctan);
  init_here(58, primitiveLogN);
  init_here(59, primitiveExp);
  init_here(60, primitiveAt);
  init_here(61, primitiveAtPut);
  init_here(62, primitiveSize);
  init_here(63, primitiveStringAt);
  init_here(64, primitiveStringAtPut);
  init_here(65, primitiveNext);
  init_here(66, primitiveNextPut);
  init_here(67, primitiveAtEnd);
  init_here(68, primitiveObjectAt);
  init_here(69, primitiveObjectAtPut);
  init_here(70, primitiveNew);
  init_here(71, primitiveNewWithArg);
  init_here(72, primitiveArrayBecomeOneWay);
  init_here(73, primitiveInstVarAt);
  init_here(74, primitiveInstVarAtPut);
  init_here(75, primitiveAsOop);
  init_here(76, primitiveStoreStackp);
  init_here(77, primitiveSomeInstance);
  init_here(78, primitiveNextInstance);
  init_here(79, primitiveNewMethod);
  init_here(80, primitiveBlockCopy);
  init_here(81, primitiveValue);
  init_here(82, primitiveValueWithArgs);
  init_here(83, primitivePerform);
  init_here(84, primitivePerformWithArgs);
  init_here(85, primitiveSignal);
  init_here(86, primitiveWait);
  init_here(87, primitiveResume);
  init_here(88, primitiveSuspend);
  init_here(89, primitiveFlushCache);
  init_main(90, primitiveMousePoint);
  init_main(91, primitiveTestDisplayDepth);
  init_main(92, primitiveSetDisplayMode);
  init_main(93, primitiveInputSemaphore);
  init_here(94, primitiveGetNextEvent);
  init_here(95, primitiveInputWord);
  init_here(96, primitiveObsoleteIndexedPrimitive);
  init_here(97, primitiveSnapshot); // try it here to avoid deadlock
  init_main(98, primitiveStoreImageSegment);
  init_main(99, primitiveLoadImageSegment);
  init_here(100, primitivePerformInSuperclass);
  init_main(101, primitiveBeCursor);
  init_here(102, primitiveBeDisplay);
  init_here(103, primitiveScanCharacters);
  init_here(104, primitiveObsoleteIndexedPrimitive);
  init_here(105, primitiveStringReplace);
  init_main(106, primitiveScreenSize);
  init_main(107, primitiveMouseButtons);
  init_main(108, primitiveKbdNext);
  init_main(109, primitiveKbdPeek);


  init_here(110, primitiveEquivalent);
  init_here(111, primitiveClass);
  init_here(112, primitiveBytesLeft);
  init_main(113, primitiveQuit);
  init_here(114, primitiveExitToDebugger);
  init_here(115, primitiveChangeClass);
  init_here(116, primitiveFlushCacheByMethod);
  // init_main(117, primitiveExternalCall); // needed to share externalPrimitiveTable to run this here instead of on main
  init_here(117, primitiveExternalCall);
  init_here(118, primitiveDoPrimitiveWithArgs);
  init_here(119, primitiveFlushCacheSelective);


  init_main(120, primitiveCalloutToFFI);
  init_main(121, primitiveImageName);
  init_here(122, primitiveNoop);
  init_here(123, primitiveValueUninterruptably);
  init_here(124, primitiveLowSpaceSemaphore);
  init_here(125, primitiveSignalAtBytesLeft);




  init_here(126, primitiveDeferDisplayUpdates);
  init_main(127, primitiveShowDisplayRect);
  init_here(128, primitiveArrayBecome);
  init_here(129, primitiveSpecialObjectsOop);
  init_here(130, primitiveFullGC);
  init_here(131, primitiveIncrementalGC);
  init_here(132, primitiveObjectPointsTo);
  init_here(133, primitiveSetInterruptKey);
  init_here(134, primitiveInterruptSemaphore);
  init_here(135, primitiveMillisecondClock);
  init_here(136, primitiveSignalAtMilliseconds);
  init_here(137, primitiveSecondsClock);
  init_here(138, primitiveSomeObject);
  init_here(139, primitiveNextObject);
  init_main(140, primitiveBeep);
  init_main(141, primitiveClipboardText);
  init_main(142, primitiveVMPath);
  init_here(143, primitiveShortAt);
  init_here(144, primitiveShortAtPut);
  init_here(145, primitiveConstantFill);
  init_here(146, primitiveObsoleteIndexedPrimitive);
  init_here(147, primitiveObsoleteIndexedPrimitive);
  init_here(148, primitiveClone);
  init_main(149, primitiveGetAttribute);


  init_here(150, 164, primitiveObsoleteIndexedPrimitive);
  init_here(165, primitiveIntegerAt);
  init_here(166, primitiveIntegerAtPut);
  init_here(167, primitiveYield);
  init_here(168, primitiveCopyObject);
  init_here(169, primitiveObsoleteIndexedPrimitive);


  init_here(170, 185, primitiveObsoleteIndexedPrimitive);


  init_here(186, primitiveFail);
  init_here(187, primitiveFail);


  init_here(188, primitiveExecuteMethodArgsArray);
  init_here(189, primitiveExecuteMethod);


  init_here(190, 194, primitiveObsoleteIndexedPrimitive);


  init_here(195, primitiveFindNextUnwindContext);
  init_here(196, primitiveTerminateTo);
  init_here(197, primitiveFindHandlerContext);
  init_here(198, primitiveMarkUnwindMethod);
  init_here(199, primitiveMarkHandlerMethod);

# if Include_Closure_Support
  init_here(200, primitiveClosureCopyWithCopiedValues);
  init_here(201, 205, primitiveClosureValue); // with 0 to 4 args
  init_here(206, primitiveClosureValueWithArgs);
  init_here(207, 209, primitiveFail); 
  init_here(210, primitiveAt); // compat w/ Cog StackInterpreterContext primitives
  init_here(211, primitiveAtPut); // compat w/ Cog StackInterpreterContext primitives
  init_here(212, primitiveSize); // compat w/ Cog StackInterpreterContext primitives
  init_here(213, 217, primitiveFail);
  init_here(218, primitiveDoNamedPrimitiveWithArgs);
  init_here(219, primitiveFail);
  init_here(220, primitiveObsoleteIndexedPrimitive);
  init_here(221, 222, primitiveClosureValueNoContextSwitch);
  init_here(223, 225, primitiveObsoleteIndexedPrimitive);
# else
  init_here(200, 225, primitiveObsoleteIndexedPrimitive);
# endif

  init_here(226, primitiveFail);
  init_here(227, primitiveFail);
  init_here(228, primitiveFail);
  init_here(229, primitiveFail);

  init_here(230, primitiveRelinquishProcessor);

  init_main(231, primitiveForceDisplayUpdate);
  init_main(232, primitiveFormPrint);
  init_main(233, primitiveSetFullScreen);
  init_here(234, primitiveObsoleteIndexedPrimitive);
  init_here(235, primitiveObsoleteIndexedPrimitive);
  init_here(236, primitiveObsoleteIndexedPrimitive);
  init_here(237, primitiveObsoleteIndexedPrimitive);
  init_here(238, 241, primitiveObsoleteIndexedPrimitive);
  init_here(242, primitiveFail);
  init_here(243, primitiveObsoleteIndexedPrimitive);
  init_here(244, primitiveObsoleteIndexedPrimitive);
  init_here(245, primitiveObsoleteIndexedPrimitive);
  init_here(246, primitiveObsoleteIndexedPrimitive);
  init_main(247, primitiveSnapshotEmbedded);
  init_here(248, primitiveInvokeObjectAsMethod);
  init_here(249, primitiveArrayBecomeOneWayCopyHash);


  init_here(250, clearProfile);
  init_here(251, dumpProfile);
  init_here(252, startProfiling);
  init_here(253, stopProfiling);
  init_main(254, primitiveVMParameter);
  init_here(255, primitiveInstVarsPutFromStack);


  init_here(256, primitivePushSelf);
  init_here(257, primitivePushTrue);
  init_here(258, primitivePushFalse);
  init_here(259, primitivePushNil);
  init_here(260, primitivePushMinusOne);
  init_here(261, primitivePushZero);
  init_here(262, primitivePushOne);
  init_here(263, primitivePushTwo);


  init_here(264, 519, primitiveLoadInstVar);

  init_here(520, primitiveFail);

  init_here(521, 529, primitiveObsoleteIndexedPrimitive);
  init_here(530, 539, primitiveFail);


  init_here(540, 545, primitiveObsoleteIndexedPrimitive);
  init_here(546, 547, primitiveFail);


  init_here(548, primitiveObsoleteIndexedPrimitive);
  init_here(549, primitiveObsoleteIndexedPrimitive);


  init_here(550, 553, primitiveObsoleteIndexedPrimitive);
  init_here(554, 569, primitiveFail);


  init_here(570, primitiveFlushExternalPrimitives);
  init_main(571, primitiveUnloadModule);
  init_main(572, primitiveListBuiltinModule);
  init_main(573, primitiveListExternalModule);
  init_here(574, primitiveFail);


  init_here(575, primitiveFail);
}



# define FWD(n) void* n(...) {The_Squeak_Interpreter()->n(); return 0; }

FOR_ALL_PRIMITIVES_DO(FWD)
# undef FOR_ALL_PRIMITIVES_DO
# undef DEF

