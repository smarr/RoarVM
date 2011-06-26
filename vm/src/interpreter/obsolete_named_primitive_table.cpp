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

Obsolete_Named_Primitive_Table obsoleteNamedPrimitiveTable;

int Obsolete_Named_Primitive_Table::find(char* name, int name_len, char* module_if_specified, int module_length_if_specified) {
  for (entry* p = contents;  p->oldName;  ++p)
    if (strncmp(p->oldName, name, name_len) == 0  &&  p->oldName[name_len] == '\0')
      if ( module_length_if_specified == 0  
      ||  (strncmp(p->plugin, module_if_specified, module_length_if_specified) == 0  &&  p->plugin[module_length_if_specified] == '\0'))
        return p - contents;
  return -1;
}

Obsolete_Named_Primitive_Table::entry Obsolete_Named_Primitive_Table::contents[] = {
  // xxxxxx most of this prims say on main but maybe many don't have to be
  {"needDummyEntryBecauseCodeTreatsZeroSpecially", "nothing", "needDummyEntryBecauseCodeTreatsZeroSpecially", true},
{"gePrimitiveMergeFillFrom", "B2DPlugin", "primitiveMergeFillFrom", true},
{"gePrimitiveSetClipRect", "B2DPlugin", "primitiveSetClipRect", true},
{"gePrimitiveDoProfileStats", "B2DPlugin", "primitiveDoProfileStats", true},
{"gePrimitiveAddCompressedShape", "B2DPlugin", "primitiveAddCompressedShape", true},
{"gePrimitiveFinishedProcessing", "B2DPlugin", "primitiveFinishedProcessing", true},
{"gePrimitiveGetBezierStats", "B2DPlugin", "primitiveGetBezierStats", true},
{"gePrimitiveSetDepth", "B2DPlugin", "primitiveSetDepth", true},
{"gePrimitiveAbortProcessing", "B2DPlugin", "primitiveAbortProcessing", true},
{"gePrimitiveGetTimes", "B2DPlugin", "primitiveGetTimes", true},
{"gePrimitiveNextActiveEdgeEntry", "B2DPlugin", "primitiveNextActiveEdgeEntry", true},
{"gePrimitiveAddBezier", "B2DPlugin", "primitiveAddBezier", true},
{"gePrimitiveRenderScanline", "B2DPlugin", "primitiveRenderScanline", true},
{"gePrimitiveAddBezierShape", "B2DPlugin", "primitiveAddBezierShape", true},
{"gePrimitiveAddLine", "B2DPlugin", "primitiveAddLine", true},
{"gePrimitiveRenderImage", "B2DPlugin", "primitiveRenderImage", true},
{"gePrimitiveGetAALevel", "B2DPlugin", "primitiveGetAALevel", true},
{"gePrimitiveRegisterExternalEdge", "B2DPlugin", "primitiveRegisterExternalEdge", true},
{"gePrimitiveInitializeBuffer", "B2DPlugin", "primitiveInitializeBuffer", true},
{"gePrimitiveAddRect", "B2DPlugin", "primitiveAddRect", true},
{"gePrimitiveInitializeProcessing", "B2DPlugin", "primitiveInitializeProcessing", true},
{"gePrimitiveAddBitmapFill", "B2DPlugin", "primitiveAddBitmapFill", true},
{"gePrimitiveGetClipRect", "B2DPlugin", "primitiveGetClipRect", true},
{"gePrimitiveGetFailureReason", "B2DPlugin", "primitiveGetFailureReason", true},
{"gePrimitiveNextGlobalEdgeEntry", "B2DPlugin", "primitiveNextGlobalEdgeEntry", true},
{"gePrimitiveNextFillEntry", "B2DPlugin", "primitiveNextFillEntry", true},
{"gePrimitiveSetColorTransform", "B2DPlugin", "primitiveSetColorTransform", true},
{"gePrimitiveDisplaySpanBuffer", "B2DPlugin", "primitiveDisplaySpanBuffer", true},
{"gePrimitiveGetOffset", "B2DPlugin", "primitiveGetOffset", true},
{"gePrimitiveAddPolygon", "B2DPlugin", "primitiveAddPolygon", true},
{"gePrimitiveNeedsFlush", "B2DPlugin", "primitiveNeedsFlush", true},
{"gePrimitiveAddOval", "B2DPlugin", "primitiveAddOval", true},
{"gePrimitiveSetAALevel", "B2DPlugin", "primitiveSetAALevel", true},
{"gePrimitiveCopyBuffer", "B2DPlugin", "primitiveCopyBuffer", true},
{"gePrimitiveAddActiveEdgeEntry", "B2DPlugin", "primitiveAddActiveEdgeEntry", true},
{"gePrimitiveGetCounts", "B2DPlugin", "primitiveGetCounts", true},
{"gePrimitiveSetOffset", "B2DPlugin", "primitiveSetOffset", true},
{"gePrimitiveAddGradientFill", "B2DPlugin", "primitiveAddGradientFill", true},
{"gePrimitiveChangedActiveEdgeEntry", "B2DPlugin", "primitiveChangedActiveEdgeEntry", true},
{"gePrimitiveRegisterExternalFill", "B2DPlugin", "primitiveRegisterExternalFill", true},
{"gePrimitiveGetDepth", "B2DPlugin", "primitiveGetDepth", true},
{"gePrimitiveSetEdgeTransform", "B2DPlugin", "primitiveSetEdgeTransform", true},
{"gePrimitiveNeedsFlushPut", "B2DPlugin", "primitiveNeedsFlushPut", true},

{"primitiveFloatArrayAt", "FloatArrayPlugin", "primitiveAt", false},
{"primitiveFloatArrayMulFloatArray", "FloatArrayPlugin", "primitiveMulFloatArray", false},
{"primitiveFloatArrayAddScalar", "FloatArrayPlugin", "primitiveAddScalar", false},
{"primitiveFloatArrayDivFloatArray", "FloatArrayPlugin", "primitiveDivFloatArray", false},
{"primitiveFloatArrayDivScalar", "FloatArrayPlugin", "primitiveDivScalar", false},
{"primitiveFloatArrayHash", "FloatArrayPlugin", "primitiveHashArray", false},
{"primitiveFloatArrayAtPut", "FloatArrayPlugin", "primitiveAtPut", false},
{"primitiveFloatArrayMulScalar", "FloatArrayPlugin", "primitiveMulScalar", false},
{"primitiveFloatArrayAddFloatArray", "FloatArrayPlugin", "primitiveAddFloatArray", false},
{"primitiveFloatArraySubScalar", "FloatArrayPlugin", "primitiveSubScalar", false},
{"primitiveFloatArraySubFloatArray", "FloatArrayPlugin", "primitiveSubFloatArray", false},
{"primitiveFloatArrayEqual", "FloatArrayPlugin", "primitiveEqual", false},
{"primitiveFloatArrayDotProduct", "FloatArrayPlugin", "primitiveDotProduct", false},

{"m23PrimitiveInvertRectInto", "Matrix2x3Plugin", "primitiveInvertRectInto", false},
{"m23PrimitiveTransformPoint", "Matrix2x3Plugin", "primitiveTransformPoint", false},
{"m23PrimitiveIsPureTranslation", "Matrix2x3Plugin", "primitiveIsPureTranslation", false},
{"m23PrimitiveComposeMatrix", "Matrix2x3Plugin", "primitiveComposeMatrix", false},
{"m23PrimitiveTransformRectInto", "Matrix2x3Plugin", "primitiveTransformRectInto", false},
{"m23PrimitiveIsIdentity", "Matrix2x3Plugin", "primitiveIsIdentity", false},
{"m23PrimitiveInvertPoint", "Matrix2x3Plugin", "primitiveInvertPoint", false},

{"primitiveDeflateBlock", "ZipPlugin", "primitiveDeflateBlock", false},
{"primitiveDeflateUpdateHashTable", "ZipPlugin", "primitiveDeflateUpdateHashTable", false},
{"primitiveUpdateGZipCrc32", "ZipPlugin", "primitiveUpdateGZipCrc32", false},
{"primitiveInflateDecompressBlock", "ZipPlugin", "primitiveInflateDecompressBlock", false},
{"primitiveZipSendBlock", "ZipPlugin", "primitiveZipSendBlock", false},

{"primitiveFFTTransformData", "FFTPlugin", "primitiveFFTTransformData", false},
{"primitiveFFTScaleData", "FFTPlugin", "primitiveFFTScaleData", false},
{"primitiveFFTPermuteData", "FFTPlugin", "primitiveFFTPermuteData", false},


  // added by me to get these to run locally -- dmu
  // also must add line in rvm_callInitializersInAllModules

  { "primitiveThisProcess",              "RVMPlugin", "primitiveThisProcess",              false },
  { "primitivePrint",                    "RVMPlugin", "primitivePrint",                    false },
  { "primitivePrintStats",               "RVMPlugin", "primitivePrintStats",               false },
  { "primitiveResetPerfCounters",        "RVMPlugin", "primitiveResetPerfCounters",        false },


  { "primitiveAllObjectsInHeap",         "RVMPlugin", "primitiveAllObjectsInHeap",         false },
  { "primitiveBreakpoint",               "RVMPlugin", "primitiveBreakpoint",               false },
  { "primitiveCoreCount",                "RVMPlugin", "primitiveCoreCount",                false },
  { "primitiveForceYields",              "RVMPlugin", "primitiveForceYields",              false },
  { "primitiveGetCore",                  "RVMPlugin", "primitiveGetCore",                  false },
  { "primitiveGetCoreIAmRunningOn",      "RVMPlugin", "primitiveGetCoreIAmRunningOn",      false },
  { "primitiveGetMutability",            "RVMPlugin", "primitiveGetMutability",            false },
  { "primitiveMoveAllToReadMostlyHeaps", "RVMPlugin", "primitiveMoveAllToReadMostlyHeaps", false },
  { "primitivePrintExecutionTrace",      "RVMPlugin", "primitivePrintExecutionTrace",      false },

  { "primitivePrintReadWriteReadMostlyBytesUsed", "RVMPlugin", "primitivePrintReadWriteReadMostlyBytesUsed", false },
  { "primitivePrintStack",               "RVMPlugin", "primitivePrintStack",               false },
  { "primitiveRunMask",                  "RVMPlugin", "primitiveRunMask",                  false },
  { "primitiveRunningProcessByCore",     "RVMPlugin", "primitiveRunningProcessByCore",     false },
  { "primitiveSampleRVM",                "RVMPlugin", "primitiveSampleRVM",                false },
  { "primitiveSetCoordinatesFor",        "RVMPlugin", "primitiveSetCoordinatesFor",        false },
  { "primitiveShuffle",                  "RVMPlugin", "primitiveShuffle",                  false },
  { "primitiveSpread",                   "RVMPlugin", "primitiveSpread",                   false },
  { "primitiveTraceCores",               "RVMPlugin", "primitiveTraceCores",               false },
  { "primitiveTraceMutatedReplicatedObjects", "RVMPlugin", "primitiveTraceMutatedReplicatedObjects", false },

  { "primitiveDebugSampleRVM",            "RVMPlugin", "primitiveDebugSampleRVM",            false },
  { "primitivePrintObjectForVMDebugging", "RVMPlugin", "primitivePrintObjectForVMDebugging", false },
  { "primitiveGetExtraPreheaderWord",     "RVMPlugin", "primitiveGetExtraPreheaderWord",     false },
  { "primitiveSetExtraPreheaderWord",     "RVMPlugin", "primitiveSetExtraPreheaderWord",     false },
  { "primitiveSetExtraWordSelector",      "RVMPlugin", "primitiveSetExtraWordSelector",      false },
  { "primitiveEmergencySemaphore",        "RVMPlugin", "primitiveEmergencySemaphore",        true  },
  { "primitiveWriteSnapshot",             "RVMPlugin", "primitiveWriteSnapshot",             true  },
  { "primitiveMicrosecondClock",          "RVMPlugin", "primitiveMicrosecondClock",          false },
  { "primitiveCycleCounter",              "RVMPlugin", "primitiveCycleCounter",              false },
  
  { "primitiveRunsHeadless",              "RVMPlugin", "primitiveRunsHeadless",              false },


  // Local versions of bitBlt primitives; don't use when blitting to the screen
  {"primitiveDrawLoopLocally", "BitBltPlugin", "primitiveDrawLoop",           On_Intel_Linux || On_Apple || false},
  {"primitiveWarpBitsLocally", "BitBltPlugin", "primitiveWarpBits",           On_Intel_Linux || On_Apple || false},
  {"copyBitsLocally", "BitBltPlugin", "copyBits",                             On_Intel_Linux || On_Apple || false},
  {"primitiveCopyBitsLocally", "BitBltPlugin", "primitiveCopyBits",           On_Intel_Linux || On_Apple || false},
  {"copyBitsFromtoatLocally", "BitBltPlugin", "copyBitsFromtoat",             On_Intel_Linux || On_Apple || false},
  {"loadBitBltFromLocally", "BitBltPlugin", "loadBitBltFrom",                 On_Intel_Linux || On_Apple || false},
  {"primitiveDisplayStringLocally", "BitBltPlugin", "primitiveDisplayString", On_Intel_Linux || On_Apple || false},



  { "primDigitMultiplyNegative", "LargeIntegers", "primDigitMultiplyNegative", false },
  { "primNormalizePositive", "LargeIntegers", "primNormalizePositive", false },
  { "primAnyBitFromTo", "LargeIntegers", "primAnyBitFromTo", false },
  { "primAsLargeInteger", "LargeIntegers", "primAsLargeInteger", false },
  { "primDigitBitAnd", "LargeIntegers", "primDigitBitAnd", false },
  { "_primDigitBitShift", "LargeIntegers", "_primDigitBitShift", false },
  { "primDigitAddWith", "LargeIntegers", "primDigitAddWith", false },
  { "primDigitCompareWith", "LargeIntegers", "primDigitCompareWith", false },
  { "primDigitSubtractWith", "LargeIntegers", "primDigitSubtractWith", false },
  { "primDigitBitShift", "LargeIntegers", "primDigitBitShift", false },
  { "primDigitSubtract", "LargeIntegers", "primDigitSubtract", false },
  { "primNormalizeNegative", "LargeIntegers", "primNormalizeNegative", false },
  { "primDigitBitLogicWithOp", "LargeIntegers", "primDigitBitLogicWithOp", false },
  { "primDigitAdd", "LargeIntegers", "primDigitAdd", false },
  { "primDigitDivWithNegative", "LargeIntegers", "primDigitDivWithNegative", false },
  { "primDigitBitShiftMagnitude", "LargeIntegers", "primDigitBitShiftMagnitude", false },
  { "primDigitBitOr", "LargeIntegers", "primDigitBitOr", false },
  { "primDigitMultiplyWithNegative", "LargeIntegers", "primDigitMultiplyWithNegative", false },
  { "primDigitBitXor", "LargeIntegers", "primDigitBitXor", false },
  { "primDigitDivNegative", "LargeIntegers", "primDigitDivNegative", false },
  { "primDigitCompare", "LargeIntegers", "primDigitCompare", false },
  { "primNormalize", "LargeIntegers", "primNormalize", false },

  { "primitiveCompareString", "MiscPrimitivePlugin", "primitiveCompareString", false },
  { "primitiveCompressToByteArray", "MiscPrimitivePlugin", "primitiveCompressToByteArray", false },
  { "primitiveDecompressFromByteArray", "MiscPrimitivePlugin", "primitiveDecompressFromByteArray", false },
  { "primitiveConvert8BitSigned", "MiscPrimitivePlugin", "primitiveConvert8BitSigned", false },
  { "primitiveFindFirstInString", "MiscPrimitivePlugin", "primitiveFindFirstInString", false },
  { "primitiveIndexOfAsciiInString", "MiscPrimitivePlugin", "primitiveIndexOfAsciiInString", false },
  { "primitiveFindSubstring", "MiscPrimitivePlugin", "primitiveFindSubstring", false },
  { "primitiveStringHash", "MiscPrimitivePlugin", "primitiveStringHash", false },
  { "primitiveTranslateStringWithTable", "MiscPrimitivePlugin", "primitiveTranslateStringWithTable", false },


  {NULL, NULL, NULL, false}
};

