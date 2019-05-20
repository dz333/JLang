// Copyright (C) 2018 Cornell University

#ifndef __JNI_H__
#define __JNI_H__

#include "class.h"
#include "factory.h"
#include "helper.h"
#include "native.h"
#include "reflect.h"
#include "rep.h"
#include "stack_trace.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

typedef uint8_t u_char;

static jboolean mainThreadIsAlive = JNI_FALSE;
static jobject mainThread = NULL;
static jobject mainThreadGroup = NULL;

[[noreturn]] static void JniUnimplemented(const char *name) {
    fprintf(stderr,
            "- - - - - - - - - - - - - - - - - - - - - - - - - - -\n"
            "The following JNI method is currently unimplemented:\n"
            "  %s\n"
            "It is defined in " __FILE__ ".\n"
            "Aborting for now.\n"
            "- - - - - - - - - - - - - - - - - - - - - - - - - - -\n",
            name);
    DumpStackTrace();
    abort();
}

// Begin helper methods.
static jstring CreateJavaString(const jchar *chars, jsize len) {
    jcharArray charArray = CreateJavaCharArray(len);
    JArrayRep *array = reinterpret_cast<JArrayRep *>(charArray);
    void *data = array->Data();
    memcpy(data, chars, sizeof(jchar) * len);
    return CreateJavaString(charArray);
}

template <typename T> static T *GetJavaStaticFieldPtr(jfieldID id) {
    auto f = reinterpret_cast<const JavaStaticFieldInfo *>(id);
    auto ptr = f->ptr;
    return reinterpret_cast<T *>(ptr);
}

// TODO check the type signature as well
template <typename T>
static void SetJavaStaticField(jclass clazz, jfieldID id, T obj) {
    *GetJavaStaticFieldPtr<T>(id) = obj;
}

template <typename T> static T *GetJavaFieldPtr(jobject obj, jfieldID id) {
    auto f = reinterpret_cast<const JavaFieldInfo *>(id);
    auto raw = reinterpret_cast<char *>(obj) + f->offset;
    return reinterpret_cast<T *>(raw);
}

template <typename T> static T GetJavaField(jobject obj, jfieldID id) {
    return *GetJavaFieldPtr<T>(obj, id);
}

template <typename T>
static void SetJavaField(jobject obj, jfieldID id, T val) {
    *GetJavaFieldPtr<T>(obj, id) = val;
}

template <typename T> static T *GetJavaArrayData(jarray arr, jboolean *isCopy) {
    // Note: several JNI methods assume no copy.
    if (isCopy != nullptr)
        *isCopy = JNI_FALSE;
    return reinterpret_cast<T *>(Unwrap(arr)->Data());
}

// Returns false if the given index is out-of-bounds.
static bool JavaArrayBoundsCheck(jarray arr, jsize index) {
    jsize len = Unwrap(arr)->Length();
    if (index < 0 || index >= len) {
        // TODO: Should technically throw ArrayIndexOutOfBoundsException.
        fprintf(stderr, "ERROR: Out-of-bounds array index %d in JNI method",
                index);
        abort();
        return false;
    }
    return true;
}

// Similar to GetJavaArrayData, but performs a
// bounds check for the specified region.
template <typename T>
static T *GetJavaArrayDataForRegion(jarray arr, jsize start, jsize len) {
    if (len == 0)
        return nullptr;
    if (!JavaArrayBoundsCheck(arr, start) ||
        !JavaArrayBoundsCheck(arr, start + len - 1))
        return nullptr;
    // Assumes no copy.
    return GetJavaArrayData<T>(arr, /*isCopy*/ nullptr) + start;
}

template <typename T>
static void GetJavaArrayRegion(jarray arr, jsize start, jsize len, T *buf) {
    if (auto data = GetJavaArrayDataForRegion<T>(arr, start, len)) {
        std::copy(data, data + len, buf);
    }
}

template <typename T>
static void SetJavaArrayRegion(jarray arr, jsize start, jsize len,
                               const T *buf) {
    if (auto data = GetJavaArrayDataForRegion<T>(arr, start, len)) {
        std::copy(buf, buf + len, data);
    }
}

// Parses one Java argument from a method signature and returns
// its single-character encoding. Updates the signature position.
static char ParseJavaArg(const char *&s) {
    switch (*s) {
    case 'Z':
    case 'B':
    case 'C':
    case 'S':
    case 'I':
    case 'J':
    case 'F':
    case 'D':
        return *s++;
    case '[':
        ParseJavaArg(++s);
        return '[';
    case 'L':
        while (*s++ != ';')
            continue;
        return 'L';
    default:
        return '\0';
    }
}

static size_t CountJavaArgs(const char *sig) {
    const char *s = sig;
    assert(*s == '(');
    ++s;

    size_t count = 0;
    while (*s != ')')
        ParseJavaArg(s), ++count;
    return count;
}

// Uses a Java method signature encoding to move
// a variable number of arguments into an array.
// Assumes the array is large enough.
static void ForwardJavaArgs(const char *sig, va_list args, jvalue *out) {
    const char *s = sig;
    jvalue *o = out;

    assert(*s == '(');
    ++s;

    static_assert(
        sizeof(jint) == sizeof(int),
        "We assume that jint is represented with int.\n"
        "This is needed to handle vararg type promotions correctly.\n"
        "See http://en.cppreference.com/w/cpp/language/variadic_arguments");

    while (*s != ')') {
        // Remember that va_arg can only be used on int, long, double,
        // and pointers. We use static casts everywhere for visual consistency.
        switch (ParseJavaArg(s)) {
        case 'Z':
            o->z = static_cast<jboolean>(va_arg(args, jint));
            break;
        case 'B':
            o->b = static_cast<jbyte>(va_arg(args, jint));
            break;
        case 'C':
            o->c = static_cast<jchar>(va_arg(args, jint));
            break;
        case 'S':
            o->s = static_cast<jshort>(va_arg(args, jint));
            break;
        case 'I':
            o->i = static_cast<jint>(va_arg(args, jint));
            break;
        case 'J':
            o->j = static_cast<jlong>(va_arg(args, jlong));
            break;
        case 'F':
            o->f = static_cast<jfloat>(va_arg(args, jdouble));
            break;
        case 'D':
            o->d = static_cast<jdouble>(va_arg(args, jdouble));
            break;
        case '[':
            o->l = static_cast<jarray>(va_arg(args, jarray));
            break;
        case 'L':
            o->l = static_cast<jobject>(va_arg(args, jobject));
            break;
        default:
            fprintf(stderr, "ERROR: Malformed method signature:\n\t%s\n", sig);
            abort();
        }
        ++o; // Advance out position.
    }
}

// This type must precisely match the type used by JLang.
template <typename T> using JniTrampolineType = T (*)(void *, const jvalue *);

// Calls a Java method directly, without using a dispatch vector.
// All other method calling functions below delegate to this one.
template <typename T>
static T CallJavaNonvirtualMethod(jmethodID id, const jvalue *args) {
    auto m = reinterpret_cast<const JavaMethodInfo *>(id);
    if (!m->fnPtr || !m->trampoline) {
        fprintf(stderr,
                "ERROR: Attempting to call Java method with trampoline %p\n"
                "       and function pointer %p\n",
                m->fnPtr, m->trampoline);
        abort();
    }
    auto trampoline = reinterpret_cast<JniTrampolineType<T>>(m->trampoline);
    return trampoline(m->fnPtr, args);
}

// Calls a Java instance method directly, without using a dispatch vector.
template <typename T>
static T CallJavaNonvirtualMethod(jobject obj, jmethodID id,
                                  const jvalue *args) {
    auto m = reinterpret_cast<const JavaMethodInfo *>(id);
    auto num_args = CountJavaArgs(m->sig);

    // Carefully include implicit receiver for non-static methods
    if (!IS_STATIC_METHOD(m)) {
        auto forward_args = std::vector<jvalue>(num_args + 1);
        forward_args[0].l = obj;
        std::copy(args, args + num_args, forward_args.begin() + 1);
        return CallJavaNonvirtualMethod<T>(id, forward_args.data());
    } else {
        return CallJavaNonvirtualMethod<T>(id, args);
    }
}

// Calls a Java instance method using the dispatch vector of [obj].
template <typename T>
static T CallJavaInstanceMethod(jobject obj, jmethodID id, const jvalue *args) {
    auto m = reinterpret_cast<const JavaMethodInfo *>(id);

    // We could implement virtual dispatch by using dispatch vector
    // offsets. However, it's slightly simpler to just look up more
    // precise method info from the class of the current object,
    // which will include a direct function pointer. Then we
    // can do a direct call.
    auto clazz = Unwrap(obj)->Cdv()->Class()->Wrap();
    m = GetJavaMethodInfo(clazz, m->name, m->sig).first;
    if (m == NULL) {
        return (T)NULL;
    }
    id = reinterpret_cast<jmethodID>(const_cast<JavaMethodInfo *>(m));

    // TODO: The above lookup may fail due to type erasure,
    // in particular if [obj] has a generic super class with substituted
    // type parameter(s) which do not match the erasure type(s).
    // See
    // https://docs.oracle.com/javase/tutorial/java/generics/bridgeMethods.html

    return CallJavaNonvirtualMethod<T>(obj, id, args);
}

template <typename T>
static T CallJavaInstanceMethod(jobject obj, const char *name, const char *sig,
                                const jvalue *args) {
    auto clazz = Unwrap(obj)->Cdv()->Class()->Wrap();
    auto m = GetJavaMethodInfo(clazz, name, sig).first;
    if (m == NULL) {
        return (T)NULL;
    }
    auto id = reinterpret_cast<jmethodID>(const_cast<JavaMethodInfo *>(m));
    return CallJavaNonvirtualMethod<T>(obj, id, args);
}

// Is exactly the same as a normal instance method, sugar for clarity
#define CallJavaConstructor(obj, id, args)                                     \
    CallJavaInstanceMethod<jobject>(obj, id, args)

// Calls a Java instance method using the dispatch vector of [obj].
template <typename T>
static T CallJavaInstanceMethod(jobject obj, jmethodID id, va_list args) {
    auto m = reinterpret_cast<const JavaMethodInfo *>(id);
    auto num_args = CountJavaArgs(m->sig);
    auto forward_args = std::vector<jvalue>(num_args);
    ForwardJavaArgs(m->sig, args, forward_args.data());
    return CallJavaInstanceMethod<T>(obj, id, forward_args.data());
}

template <typename T>
static T CallJavaInterfaceMethod(jobject obj, jclass intf, const char *name,
                                 const char *sig, const jvalue *args) {
    auto methodInfoPair = GetJavaMethodInfo(intf, name, sig);
    auto methodInfo = methodInfoPair.first;
    if (methodInfo == NULL) {
        return (T)NULL;
    }
    auto methodIndex = methodInfoPair.second;
    void *methodToCall = __getInterfaceMethod(obj, methodInfo->intf_id_hash,
                                              methodInfo->intf_id, methodIndex);
    return ((T(*)(jobject))methodToCall)(obj);
}

// Calls a Java instance method using the dispatch vector of [obj].
template <typename T>
static T CallJavaStaticMethod(jclass cls, jmethodID id, va_list args) {
    auto m = reinterpret_cast<const JavaMethodInfo *>(id);
    auto num_args = CountJavaArgs(m->sig);
    auto forward_args = std::vector<jvalue>(num_args);
    ForwardJavaArgs(m->sig, args, forward_args.data());
    return CallJavaNonvirtualMethod<T>(id, forward_args.data());
}

static jobjectArray GetJavaConstructors(jclass clazz, const JavaClassInfo *info,
                                        jboolean publicOnly) {
    // TODO pass public/private info to runtime, for now ignore the publicOnly
    int ctor_count = 0;
    for (int i = 0; i < info->num_methods; i++) {
        if (IS_CONSTRUCTOR(&(info->methods[i]))) {
            ctor_count++;
        }
    }
    jobjectArray res = (jobjectArray)create1DArray(
        "[Ljava.lang.reflect.Constructor;", ctor_count);
    int ctors_copied = 0;
    for (int i = 0; ctors_copied < ctor_count; i++) {
        if (IS_CONSTRUCTOR(&(info->methods[i]))) {
            POLYGLOT_ARRAY_STORE(
                res, (jint)ctors_copied,
                CreateConstructor(clazz, info, info->methods[i]));
            ctors_copied++;
        }
    }
    return res;
}

static jobject GetMainThread() {
    if (mainThread == NULL) {
        if (mainThreadGroup == NULL) {
            auto thread_group_clazz =
                LoadJavaClassFromLib("java.lang.ThreadGroup");
            mainThreadGroup = CreateJavaObject(thread_group_clazz);
            CallJavaInstanceMethod<jobject>(mainThreadGroup, "<init>", "()V",
                                            NULL);
        }
        auto thread_clazz = GetJavaClassFromName("java.lang.Thread");
        mainThread = CreateJavaObject(thread_clazz);
        const jobject mainThreadArgs[] = {mainThreadGroup, NULL, NULL};
        CallJavaInstanceMethod<jobject>(
            mainThread, "<init>",
            "(Ljava/lang/ThreadGroup;Ljava/lang/Runnable;Ljava/lang/Thread;)V",
            reinterpret_cast<const jvalue *>(mainThreadArgs));
        mainThreadIsAlive = JNI_TRUE;
    }
    return mainThread;
}

// Copied from JDK implementation, TODO replace with our own impl
// UTF8 helpers
// Writes a jchar a utf8 and returns the end
static u_char *utf8_write(u_char *base, jchar ch) {
    if ((ch != 0) && (ch <= 0x7f)) {
        base[0] = (u_char)ch;
        return base + 1;
    }

    if (ch <= 0x7FF) {
        /* 11 bits or less. */
        unsigned char high_five = ch >> 6;
        unsigned char low_six = ch & 0x3F;
        base[0] = high_five | 0xC0; /* 110xxxxx */
        base[1] = low_six | 0x80;   /* 10xxxxxx */
        return base + 2;
    }
    /* possibly full 16 bits. */
    char high_four = ch >> 12;
    char mid_six = (ch >> 6) & 0x3F;
    char low_six = ch & 0x3f;
    base[0] = high_four | 0xE0; /* 1110xxxx */
    base[1] = mid_six | 0x80;   /* 10xxxxxx */
    base[2] = low_six | 0x80;   /* 10xxxxxx */
    return base + 3;
}

static int utf8_length(jchar *base, int length) {
    int result = 0;
    for (int index = 0; index < length; index++) {
        jchar c = base[index];
        if ((0x0001 <= c) && (c <= 0x007F))
            result += 1;
        else if (c <= 0x07FF)
            result += 2;
        else
            result += 3;
    }
    return result;
}

static char *as_utf8(jchar *base, int length, u_char *result) {
    int utf8_len = utf8_length(base, length);
    u_char *p = result;
    for (int index = 0; index < length; index++) {
        p = utf8_write(p, base[index]);
    }
    *p = '\0';
    assert(p == &result[utf8_len]);
    return (char *)result;
}
#endif
