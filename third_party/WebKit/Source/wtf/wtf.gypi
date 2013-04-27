{
    'variables': {
        'wtf_files': [
            'ASCIICType.h',
            'AVLTree.h',
            'Alignment.h',
            'ArrayBuffer.cpp',
            'ArrayBuffer.h',
            'ArrayBufferView.cpp',
            'ArrayBufferView.h',
            'Assertions.cpp',
            'Assertions.h',
            'Atomics.h',
            'AutodrainedPool.h',
            'AutodrainedPoolMac.mm',
            'BitArray.h',
            'BitVector.cpp',
            'BitVector.h',
            'Bitmap.h',
            'BlockStack.h',
            'BloomFilter.h',
            'ByteOrder.h',
            'CheckedArithmetic.h',
            'CheckedBoolean.h',
            'Compiler.h',
            'Complex.h',
            'CryptographicallyRandomNumber.cpp',
            'CryptographicallyRandomNumber.h',
            'CurrentTime.h',
            'DataLog.cpp',
            'DataLog.h',
            'DateMath.cpp',
            'DateMath.h',
            'DecimalNumber.cpp',
            'DecimalNumber.h',
            'Decoder.h',
            'Deque.h',
            'DoublyLinkedList.h',
            'DynamicAnnotations.cpp',
            'DynamicAnnotations.h',
            'Encoder.h',
            'FastAllocBase.h',
            'FastMalloc.cpp',
            'FastMalloc.h',
            'FilePrintStream.cpp',
            'FilePrintStream.h',
            'FixedArray.h',
            'Float32Array.h',
            'Float64Array.h',
            'Forward.h',
            'Functional.h',
            'GetPtr.h',
            'GregorianDateTime.cpp',
            'GregorianDateTime.h',
            'HashCountedSet.h',
            'HashFunctions.h',
            'HashIterators.h',
            'HashMap.h',
            'HashSet.h',
            'HashTable.cpp',
            'HashTable.h',
            'HashTraits.h',
            'HexNumber.h',
            'Int16Array.h',
            'Int32Array.h',
            'Int8Array.h',
            'IntegralTypedArrayBase.h',
            'ListHashSet.h',
            'ListRefPtr.h',
            'Locker.h',
            'MD5.cpp',
            'MD5.h',
            'MainThread.h',
            'MallocZoneSupport.h',
            'MathExtras.h',
            'MediaTime.cpp',
            'MediaTime.h',
            'MemoryInstrumentation.cpp',
            'MemoryInstrumentation.h',
            'MemoryInstrumentationArrayBufferView.h',
            'MemoryInstrumentationHashCountedSet.h',
            'MemoryInstrumentationHashMap.h',
            'MemoryInstrumentationHashSet.h',
            'MemoryInstrumentationListHashSet.h',
            'MemoryInstrumentationSequence.h',
            'MemoryInstrumentationString.h',
            'MemoryInstrumentationVector.h',
            'MemoryObjectInfo.h',
            'MessageQueue.h',
            'NonCopyingSort.h',
            'Noncopyable.h',
            'NotFound.h',
            'NullPtr.cpp',
            'NullPtr.h',
            'NumberOfCores.cpp',
            'NumberOfCores.h',
            'OSRandomSource.h',
            'OwnArrayPtr.h',
            'OwnPtr.h',
            'OwnPtrCommon.h',
            'ParallelJobs.h',
            'ParallelJobsGeneric.cpp',
            'ParallelJobsGeneric.h',
            'ParallelJobsLibdispatch.h',
            'ParallelJobsOpenMP.h',
            'PassOwnArrayPtr.h',
            'PassOwnPtr.h',
            'PassRefPtr.h',
            'PassTraits.h',
            'Platform.h',
            'PossiblyNull.h',
            'PrintStream.cpp',
            'PrintStream.h',
            'ProcessID.h',
            'RandomNumber.cpp',
            'RandomNumber.h',
            'RandomNumberSeed.h',
            'RawPointer.h',
            'RefCounted.h',
            'RefCountedLeakCounter.cpp',
            'RefCountedLeakCounter.h',
            'RefPtr.h',
            'RefPtrHashMap.h',
            'RetainPtr.h',
            'SHA1.cpp',
            'SHA1.h',
            'SaturatedArithmetic.h',
            'SinglyLinkedList.h',
            'SizeLimits.cpp',
            'StackBounds.cpp',
            'StackBounds.h',
            'StaticConstructors.h',
            'StdLibExtras.h',
            'StringExtras.h',
            'StringHasher.h',
            'TCPackedCache.h',
            'TCPageMap.h',
            'TCSpinLock.h',
            'TCSystemAlloc.cpp',
            'TCSystemAlloc.h',
            'TemporaryChange.h',
            'ThreadFunctionInvocation.h',
            'ThreadIdentifierDataPthreads.cpp',
            'ThreadIdentifierDataPthreads.h',
            'ThreadRestrictionVerifier.h',
            'ThreadSafeRefCounted.h',
            'ThreadSpecific.h',
            'ThreadSpecificWin.cpp',
            'Threading.cpp',
            'Threading.h',
            'ThreadingPrimitives.h',
            'ThreadingPthreads.cpp',
            'ThreadingWin.cpp',
            'TypeTraits.cpp',
            'TypeTraits.h',
            'TypedArrayBase.h',
            'Uint16Array.h',
            'Uint32Array.h',
            'Uint8Array.h',
            'UnusedParam.h',
            'VMTags.h',
            'ValueCheck.h',
            'Vector.h',
            'VectorTraits.h',
            'WTFThreadData.cpp',
            'WTFThreadData.h',
            'WeakPtr.h',
            'chromium/ChromiumThreading.h',
            'chromium/MainThreadChromium.cpp',
            'dtoa.cpp',
            'dtoa.h',
            'dtoa/bignum-dtoa.cc',
            'dtoa/bignum-dtoa.h',
            'dtoa/bignum.cc',
            'dtoa/bignum.h',
            'dtoa/cached-powers.cc',
            'dtoa/cached-powers.h',
            'dtoa/diy-fp.cc',
            'dtoa/diy-fp.h',
            'dtoa/double-conversion.cc',
            'dtoa/double-conversion.h',
            'dtoa/double.h',
            'dtoa/fast-dtoa.cc',
            'dtoa/fast-dtoa.h',
            'dtoa/fixed-dtoa.cc',
            'dtoa/fixed-dtoa.h',
            'dtoa/strtod.cc',
            'dtoa/strtod.h',
            'dtoa/utils.h',
            'text/ASCIIFastPath.h',
            'text/AtomicString.cpp',
            'text/AtomicString.h',
            'text/AtomicStringHash.h',
            'text/AtomicStringImpl.h',
            'text/Base64.cpp',
            'text/Base64.h',
            'text/CString.cpp',
            'text/CString.h',
            'text/IntegerToStringConversion.h',
            'text/StringBuffer.h',
            'text/StringBuilder.cpp',
            'text/StringBuilder.h',
            'text/StringConcatenate.h',
            'text/StringHash.h',
            'text/StringImpl.cpp',
            'text/StringImpl.h',
            'text/StringOperators.h',
            'text/StringStatics.cpp',
            'text/TextPosition.h',
            'text/WTFString.cpp',
            'text/WTFString.h',
            'unicode/CharacterNames.h',
            'unicode/Collator.h',
            'unicode/CollatorDefault.cpp',
            'unicode/ScriptCodesFromICU.h',
            'unicode/UTF8.cpp',
            'unicode/UTF8.h',
            'unicode/Unicode.h',
            'unicode/UnicodeMacrosFromICU.h',
            'unicode/icu/CollatorICU.cpp',
            'unicode/icu/UnicodeIcu.h',
        ],
        'wtf_unittest_files': [
            'tests/AtomicString.cpp',
            'tests/CheckedArithmeticOperations.cpp',
            'tests/CString.cpp',
            'tests/Functional.cpp',
            'tests/HashMap.cpp',
            'tests/HashSet.cpp',
            'tests/ListHashSet.cpp',
            'tests/MathExtras.cpp',
            'tests/MediaTime.cpp',
            'tests/MemoryInstrumentationTest.cpp',
            'tests/SaturatedArithmeticOperations.cpp',
            'tests/StringBuilder.cpp',
            'tests/StringHasher.cpp',
            'tests/StringImpl.cpp',
            'tests/StringOperators.cpp',
            'tests/TemporaryChange.cpp',
            'tests/Vector.cpp',
            'tests/VectorBasic.cpp',
            'tests/VectorReverse.cpp',
            'tests/WTFString.cpp',
        ],
    },
}
