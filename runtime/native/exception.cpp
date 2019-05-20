// Copyright (C) 2018 Cornell University

#include "exception.h"

#include <cstdio>
#include <cstring>
#include <inttypes.h>
#include <stdexcept>
#include <unwind.h>

#include <cassert>
#include <gc.h>

#include "reflect.h"
#include "stack_trace.h"

// Large chunks of this file are from ExceptionDemo.cpp in llvm/examples,
// and some chunks are from llvm/BinaryFormat/Dwarf.h.

namespace llvm {
namespace dwarf {
enum Constants {
    DW_CHILDREN_no = 0x00,
    DW_CHILDREN_yes = 0x01,

    DW_EH_PE_absptr = 0x00,
    DW_EH_PE_omit = 0xff,
    DW_EH_PE_uleb128 = 0x01,
    DW_EH_PE_udata2 = 0x02,
    DW_EH_PE_udata4 = 0x03,
    DW_EH_PE_udata8 = 0x04,
    DW_EH_PE_sleb128 = 0x09,
    DW_EH_PE_sdata2 = 0x0A,
    DW_EH_PE_sdata4 = 0x0B,
    DW_EH_PE_sdata8 = 0x0C,
    DW_EH_PE_signed = 0x08,
    DW_EH_PE_pcrel = 0x10,
    DW_EH_PE_textrel = 0x20,
    DW_EH_PE_datarel = 0x30,
    DW_EH_PE_funcrel = 0x40,
    DW_EH_PE_aligned = 0x50,
    DW_EH_PE_indirect = 0x80
};
}
} // namespace llvm

namespace {
template <typename Type_> uintptr_t ReadType(const uint8_t *&p) {
    Type_ value;
    memcpy(&value, p, sizeof(Type_));
    p += sizeof(Type_);
    return static_cast<uintptr_t>(value);
}
} // namespace

extern "C" {

void
    Polyglot_jlang_runtime_Exceptions_createClassNotFoundException__Ljava_lang_String_2(
        jstring);
void Polyglot_jlang_runtime_Exceptions_throwNewThrowable__Ljava_lang_Class_2Ljava_lang_String_2(
    jclass clazz, jstring str);
void Polyglot_jlang_runtime_Exceptions_throwThrowable__Ljava_lang_Throwable_2(
    jthrowable obj);
// A distinct integer identifying our own exceptions.
const uint64_t javaExceptionClass = 8101813523428701805ll;

struct JavaException_t {
    jobject jexception;

    // Note: This is properly aligned in unwind.h
    _Unwind_Exception unwindException;
};

typedef struct _Unwind_Context *_Unwind_Context_t;

void deleteJavaException(_Unwind_Reason_Code reason,
                         _Unwind_Exception *expToDelete) {
    return; // The exception will be deleted by the garbage collector
}

_Unwind_Exception *createUnwindException(jobject jexception) {
    JavaException_t *ret =
        (JavaException_t *)GC_malloc(sizeof(JavaException_t));
    ret->jexception = jexception;
    ret->unwindException.exception_class = javaExceptionClass;
    ret->unwindException.exception_cleanup = deleteJavaException;
    return &ret->unwindException;
}

void throwUnwindException(_Unwind_Exception *exception) {
    _Unwind_RaiseException(exception);
    fprintf(stderr, "- - - - - - - - - - - - - - - - - -\n"
                    "Aborting due to uncaught exception.\n"
                    "- - - - - - - - - - - - - - - - - -\n");
    DumpStackTrace();
    fflush(stderr);
    abort();
}

JavaException_t *extractJavaException(_Unwind_Exception *unwindException) {
    struct JavaException_t dummyException;
    int64_t ourBaseFromUnwindOffset =
        ((uintptr_t)&dummyException) -
        ((uintptr_t)&dummyException.unwindException);

    JavaException_t *exn =
        (struct JavaException_t *)(((char *)unwindException) +
                                   ourBaseFromUnwindOffset);
    return exn;
}

jobject extractJavaExceptionObject(_Unwind_Exception *unwindException) {
    return extractJavaException(unwindException)->jexception;
}

/// Read a uleb128 encoded value and advance pointer
/// See Variable Length Data in:
/// @link http://dwarfstd.org/Dwarf3.pdf @unlink
/// @param data reference variable holding memory pointer to decode from
/// @returns decoded value
static uintptr_t readULEB128(const uint8_t **data) {
    uintptr_t result = 0;
    uintptr_t shift = 0;
    unsigned char byte;
    const uint8_t *p = *data;

    do {
        byte = *p++;
        result |= (byte & 0x7f) << shift;
        shift += 7;
    } while (byte & 0x80);

    *data = p;

    return result;
}

/// Read a sleb128 encoded value and advance pointer
/// See Variable Length Data in:
/// @link http://dwarfstd.org/Dwarf3.pdf @unlink
/// @param data reference variable holding memory pointer to decode from
/// @returns decoded value
static uintptr_t readSLEB128(const uint8_t **data) {
    uintptr_t result = 0;
    uintptr_t shift = 0;
    unsigned char byte;
    const uint8_t *p = *data;

    do {
        byte = *p++;
        result |= (byte & 0x7f) << shift;
        shift += 7;
    } while (byte & 0x80);

    *data = p;

    if ((byte & 0x40) && (shift < (sizeof(result) << 3))) {
        result |= (~0 << shift);
    }

    return result;
}

unsigned getEncodingSize(uint8_t Encoding) {
    if (Encoding == llvm::dwarf::DW_EH_PE_omit)
        return 0;

    switch (Encoding & 0x0F) {
    case llvm::dwarf::DW_EH_PE_absptr:
        return sizeof(uintptr_t);
    case llvm::dwarf::DW_EH_PE_udata2:
        return sizeof(uint16_t);
    case llvm::dwarf::DW_EH_PE_udata4:
        return sizeof(uint32_t);
    case llvm::dwarf::DW_EH_PE_udata8:
        return sizeof(uint64_t);
    case llvm::dwarf::DW_EH_PE_sdata2:
        return sizeof(int16_t);
    case llvm::dwarf::DW_EH_PE_sdata4:
        return sizeof(int32_t);
    case llvm::dwarf::DW_EH_PE_sdata8:
        return sizeof(int64_t);
    default:
        // not supported
        abort();
    }
}

/// Read a pointer encoded value and advance pointer
/// See Variable Length Data in:
/// @link http://dwarfstd.org/Dwarf3.pdf @unlink
/// @param data reference variable holding memory pointer to decode from
/// @param encoding dwarf encoding type
/// @returns decoded value
static uintptr_t readEncodedPointer(const uint8_t **data, uint8_t encoding,
                                    bool shortCircuitNull = false) {
    uintptr_t result = 0;
    const uint8_t *p = *data;

    if (encoding == llvm::dwarf::DW_EH_PE_omit)
        return (result);

    // first get value
    switch (encoding & 0x0F) {
    case llvm::dwarf::DW_EH_PE_absptr:
        result = ReadType<uintptr_t>(p);
        break;
    case llvm::dwarf::DW_EH_PE_uleb128:
        result = readULEB128(&p);
        break;
    // Note: This case has not been tested
    case llvm::dwarf::DW_EH_PE_sleb128:
        result = readSLEB128(&p);
        break;
    case llvm::dwarf::DW_EH_PE_udata2:
        result = ReadType<uint16_t>(p);
        break;
    case llvm::dwarf::DW_EH_PE_udata4:
        result = ReadType<uint32_t>(p);
        break;
    case llvm::dwarf::DW_EH_PE_udata8:
        result = ReadType<uint64_t>(p);
        break;
    case llvm::dwarf::DW_EH_PE_sdata2:
        result = ReadType<int16_t>(p);
        break;
    case llvm::dwarf::DW_EH_PE_sdata4:
        result = ReadType<int32_t>(p);
        break;
    case llvm::dwarf::DW_EH_PE_sdata8:
        result = ReadType<int64_t>(p);
        break;
    default:
        // not supported
        abort();
        break;
    }

    // Warning: added to support "catch i8* null", but without
    //          fully understanding why this works.
    if (shortCircuitNull && !result)
        return result;

    // then add relative offset
    switch (encoding & 0x70) {
    case llvm::dwarf::DW_EH_PE_absptr:
        // do nothing
        break;
    case llvm::dwarf::DW_EH_PE_pcrel:
        result += (uintptr_t)(*data);
        break;
    case llvm::dwarf::DW_EH_PE_textrel:
    case llvm::dwarf::DW_EH_PE_datarel:
    case llvm::dwarf::DW_EH_PE_funcrel:
    case llvm::dwarf::DW_EH_PE_aligned:
    default:
        // not supported
        abort();
        break;
    }

    // then apply indirection
    if (encoding & llvm::dwarf::DW_EH_PE_indirect) {
        result = *((uintptr_t *)result);
    }

    *data = p;

    return result;
}

/// Deals with Dwarf actions matching our type infos
/// (OurExceptionType_t instances). Returns whether or not a dwarf emitted
/// action matches the supplied exception type. If such a match succeeds,
/// the resultAction argument will be set with > 0 index value. Only
/// corresponding llvm.eh.selector type info arguments, cleanup arguments
/// are supported. Filters are not supported.
/// See Variable Length Data in:
/// @link http://dwarfstd.org/Dwarf3.pdf @unlink
/// Also see @link http://mentorembedded.github.com/cxx-abi/abi-eh.html @unlink
/// @param resultAction reference variable which will be set with result
/// @param classInfo our array of type info pointers (to globals)
/// @param actionEntry index into above type info array or 0 (clean up).
///        We do not support filters.
/// @param exceptionClass exception class (_Unwind_Exception::exception_class)
///        of thrown exception.
/// @param exceptionObject thrown _Unwind_Exception instance.
/// @returns whether or not a type info was found. False is returned if only
///          a cleanup was found
static bool handleActionValue(int64_t *resultAction, uint8_t TTypeEncoding,
                              const uint8_t *ClassInfo, uintptr_t actionEntry,
                              uint64_t exceptionClass,
                              struct _Unwind_Exception *exceptionObject) {
    bool ret = false;

    if (!resultAction || !exceptionObject ||
        (exceptionClass != javaExceptionClass))
        return (ret);

    struct JavaException_t *exn = extractJavaException(exceptionObject);

    const uint8_t *actionPos = (uint8_t *)actionEntry, *tempActionPos;
    int64_t typeOffset = 0, actionOffset;

    for (int i = 0; true; ++i) {
        // Each emitted dwarf action corresponds to a 2 tuple of
        // type info address offset, and action offset to the next
        // emitted action.
        typeOffset = readSLEB128(&actionPos);
        tempActionPos = actionPos;
        actionOffset = readSLEB128(&tempActionPos);

        assert((typeOffset >= 0) &&
               "handleActionValue(...):filters are not supported.");

        // Note: A typeOffset == 0 implies that a cleanup llvm.eh.selector
        //       argument has been matched.
        if (typeOffset > 0) {
            unsigned EncSize = getEncodingSize(TTypeEncoding);
            const uint8_t *EntryP = ClassInfo - typeOffset * EncSize;
            uintptr_t P = readEncodedPointer(&EntryP, TTypeEncoding,
                                             /*shortCircuitNull*/ true);
            void *ThisClassInfo = reinterpret_cast<void *>(P);

            // Recall that a null class info represents a catch-all clause.
            if (!ThisClassInfo || InstanceOf(exn->jexception, ThisClassInfo)) {
                *resultAction = i + 1;
                ret = true;
                break;
            }
        }

        if (!actionOffset)
            break;

        actionPos += actionOffset;
    }

    return (ret);
}

/// Deals with the Language specific data portion of the emitted dwarf code.
/// See @link http://itanium-cxx-abi.github.io/cxx-abi/abi-eh.html @unlink
/// @param version unsupported (ignored), unwind version
/// @param lsda language specific data area
/// @param _Unwind_Action actions minimally supported unwind stage
///        (forced specifically not supported)
/// @param exceptionClass exception class (_Unwind_Exception::exception_class)
///        of thrown exception.
/// @param exceptionObject thrown _Unwind_Exception instance.
/// @param context unwind system context
/// @returns minimally supported unwinding control indicator
static _Unwind_Reason_Code handleLsda(int version, const uint8_t *lsda,
                                      _Unwind_Action actions,
                                      uint64_t exceptionClass,
                                      struct _Unwind_Exception *exceptionObject,
                                      struct _Unwind_Context *context) {
    _Unwind_Reason_Code ret = _URC_CONTINUE_UNWIND;

    if (!lsda)
        return (ret);

    // Get the current instruction pointer and offset it before next
    // instruction in the current frame which threw the exception.
    uintptr_t pc = _Unwind_GetIP(context) - 1;

    // Get beginning current frame's code (as defined by the
    // emitted dwarf code)
    uintptr_t funcStart = _Unwind_GetRegionStart(context);
    uintptr_t pcOffset = pc - funcStart;
    const uint8_t *ClassInfo = NULL;

    // Note: See JITDwarfEmitter::EmitExceptionTable(...) for corresponding
    //       dwarf emission

    // Parse LSDA header.
    uint8_t lpStartEncoding = *lsda++;

    if (lpStartEncoding != llvm::dwarf::DW_EH_PE_omit) {
        readEncodedPointer(&lsda, lpStartEncoding);
    }

    uint8_t ttypeEncoding = *lsda++;
    uintptr_t classInfoOffset;

    if (ttypeEncoding != llvm::dwarf::DW_EH_PE_omit) {
        // Calculate type info locations in emitted dwarf code which
        // were flagged by type info arguments to llvm.eh.selector
        // intrinsic
        classInfoOffset = readULEB128(&lsda);
        ClassInfo = lsda + classInfoOffset;
    }

    // Walk call-site table looking for range that
    // includes current PC.

    uint8_t callSiteEncoding = *lsda++;
    uint32_t callSiteTableLength = readULEB128(&lsda);
    const uint8_t *callSiteTableStart = lsda;
    const uint8_t *callSiteTableEnd = callSiteTableStart + callSiteTableLength;
    const uint8_t *actionTableStart = callSiteTableEnd;
    const uint8_t *callSitePtr = callSiteTableStart;

    while (callSitePtr < callSiteTableEnd) {
        uintptr_t start = readEncodedPointer(&callSitePtr, callSiteEncoding);
        uintptr_t length = readEncodedPointer(&callSitePtr, callSiteEncoding);
        uintptr_t landingPad =
            readEncodedPointer(&callSitePtr, callSiteEncoding);

        // Note: Action value
        uintptr_t actionEntry = readULEB128(&callSitePtr);

        if (exceptionClass != javaExceptionClass) {
            // We have been notified of a foreign exception being thrown,
            // and we therefore need to execute cleanup landing pads
            actionEntry = 0;
        }

        if (landingPad == 0) {
            continue; // no landing pad for this entry
        }

        if (actionEntry) {
            actionEntry += ((uintptr_t)actionTableStart) - 1;
        }

        bool exceptionMatched = false;

        if ((start <= pcOffset) && (pcOffset < (start + length))) {
            int64_t actionValue = 0;

            if (actionEntry) {
                exceptionMatched = handleActionValue(
                    &actionValue, ttypeEncoding, ClassInfo, actionEntry,
                    exceptionClass, exceptionObject);
            }

            if (!(actions & _UA_SEARCH_PHASE)) {

                // Found landing pad for the PC.
                // Set Instruction Pointer to so we re-enter function
                // at landing pad. The landing pad is created by the
                // compiler to take two parameters in registers.
                _Unwind_SetGR(context, __builtin_eh_return_data_regno(0),
                              (uintptr_t)exceptionObject);

                // Note: this virtual register directly corresponds
                //       to the return of the llvm.eh.selector intrinsic
                if (!actionEntry || !exceptionMatched) {
                    // We indicate cleanup only
                    _Unwind_SetGR(context, __builtin_eh_return_data_regno(1),
                                  0);
                } else {
                    // Matched type info index of llvm.eh.selector intrinsic
                    // passed here.
                    _Unwind_SetGR(context, __builtin_eh_return_data_regno(1),
                                  actionValue);
                }

                // To execute landing pad set here
                _Unwind_SetIP(context, funcStart + landingPad);
                ret = _URC_INSTALL_CONTEXT;
            } else if (exceptionMatched) {
                ret = _URC_HANDLER_FOUND;
            } else {
                // Note: Only non-clean up handlers are marked as
                //       found. Otherwise the clean up handlers will be
                //       re-found and executed during the clean up
                //       phase.
            }

            break;
        }
    }

    return (ret);
}

/// This is the personality function which is embedded (dwarf emitted), in the
/// dwarf unwind info block. Again see: JITDwarfEmitter.cpp.
/// See @link http://itanium-cxx-abi.github.io/cxx-abi/abi-eh.html @unlink
/// @param version unsupported (ignored), unwind version
/// @param _Unwind_Action actions minimally supported unwind stage
///        (forced specifically not supported)
/// @param exceptionClass exception class (_Unwind_Exception::exception_class)
///        of thrown exception.
/// @param exceptionObject thrown _Unwind_Exception instance.
/// @param context unwind system context
/// @returns minimally supported unwinding control indicator
_Unwind_Reason_Code
__java_personality_v0(int version, _Unwind_Action actions,
                      uint64_t exceptionClass,
                      struct _Unwind_Exception *exceptionObject,
                      struct _Unwind_Context *context) {
    const uint8_t *lsda = reinterpret_cast<const uint8_t *>(
        _Unwind_GetLanguageSpecificData(context));
    return handleLsda(version, lsda, actions, exceptionClass, exceptionObject,
                      context);
}

} // extern "C"

void throwClassNotFoundException(JNIEnv *env, const char *name) {
    jstring clazz = env->NewStringUTF(name);
    Polyglot_jlang_runtime_Exceptions_createClassNotFoundException__Ljava_lang_String_2(
        clazz);
}

void throwNewThrowable(JNIEnv *env, jclass clazz, const char *msg) {
    jstring msgStr = env->NewStringUTF(msg);
    Polyglot_jlang_runtime_Exceptions_throwNewThrowable__Ljava_lang_Class_2Ljava_lang_String_2(
        clazz, msgStr);
}

void throwThrowable(JNIEnv *env, jthrowable obj) {
    Polyglot_jlang_runtime_Exceptions_throwThrowable__Ljava_lang_Throwable_2(
        obj);
}
