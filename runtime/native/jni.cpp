// Copyright (C) 2018 Cornell University

#include "class.h"
#include "exception.h"
#include "jni_help.h"
#include "reflect.h"
// Begin official API.

extern "C" {

jint jni_GetVersion(JNIEnv *env) {
    // This is the correct version for Java SE 7 too.
    return JNI_VERSION_1_6;
}

jclass jni_DefineClass(JNIEnv *env, const char *name, jobject loader,
                       const jbyte *buf, jsize len) {
    JniUnimplemented("DefineClass");
}

/* name is in the java/lang/String format and is a valid C-string */
jclass jni_FindClass(JNIEnv *env, const char *name) {
    // TODO this should do a classpath (java.class.path) search for compiled
    // files
    if (name == NULL)
        return NULL;
    jclass clazz = GetJavaClassFromPathName(name);
    if (clazz == NULL) {
        return LoadJavaClassFromLib(name);
    } else {
        return clazz;
    }
}

jmethodID jni_FromReflectedMethod(JNIEnv *env, jobject method) {
    JniUnimplemented("FromReflectedMethod");
}

jfieldID jni_FromReflectedField(JNIEnv *env, jobject field) {
    JniUnimplemented("FromReflectedField");
}

jobject jni_ToReflectedMethod(JNIEnv *env, jclass cls, jmethodID id,
                              jboolean isStatic) {
    JniUnimplemented("ToReflectedMethod");
}

jclass jni_GetSuperclass(JNIEnv *env, jclass sub) {
    const JavaClassInfo *info = GetJavaClassInfo(sub);
    if (info->super_ptr == NULL) {
        return NULL;
    }
    return *(info->super_ptr);
    // JniUnimplemented("GetSuperclass");
}

jboolean jni_IsAssignableFrom(JNIEnv *env, jclass sup, jclass sub) {
    // dw475 TODO return sub <? sup
    // return JNI_TRUE;
    if (sub == NULL) {
        // return JNI_FALSE;
        // throw null pointer
    }
    // there exists a superclass or superinterface of sub = sup
    return JNI_TRUE;
    JniUnimplemented("jni_IsAssignableFrom");
}

jobject jni_ToReflectedField(JNIEnv *env, jclass cls, jfieldID id,
                             jboolean isStatic) {
    JniUnimplemented("ToReflectedField");
}

jint jni_Throw(JNIEnv *env, jthrowable obj) {
    throwThrowable(env, obj);
    return -1;
}

jint jni_ThrowNew(JNIEnv *env, jclass clazz, const char *msg) {
    throwNewThrowable(env, clazz, msg);
    return -1;
}

jthrowable jni_ExceptionOccurred(JNIEnv *env) {
    // TODO do something here (ExceptionOccurred)
    return NULL;
}

void jni_ExceptionDescribe(JNIEnv *env) {
    JniUnimplemented("ExceptionDescribe");
}

void jni_ExceptionClear(JNIEnv *env) { JniUnimplemented("ExceptionClear"); }

void jni_FatalError(JNIEnv *env, const char *msg) {
    JniUnimplemented("FatalError");
}

jint jni_PushLocalFrame(JNIEnv *env, jint capacity) {
    JniUnimplemented("PushLocalFrame");
}

jobject jni_PopLocalFrame(JNIEnv *env, jobject result) {
    JniUnimplemented("PopLocalFrame");
}

jobject jni_NewGlobalRef(JNIEnv *env, jobject lobj) {
    // TODO, do something real here for global ref
    return lobj;
}

void jni_DeleteGlobalRef(JNIEnv *env, jobject gref) {
    // TODO handle delete global ref if necessary
    //    JniUnimplemented("DeleteGlobalRef");
}

void jni_DeleteLocalRef(JNIEnv *env, jobject obj) {
    // TODO handle DeleteLocalRef if necessary (or remove TODO if not)
    //    JniUnimplemented("DeleteLocalRef");
}

jboolean jni_IsSameObject(JNIEnv *env, jobject obj1, jobject obj2) {
    return obj1 == obj2;
}

jobject jni_NewLocalRef(JNIEnv *env, jobject ref) {
    JniUnimplemented("NewLocalRef");
}

#define MAX_CAPACITY 4000
jint jni_EnsureLocalCapacity(JNIEnv *env, jint capacity) {
    // JDK actually just does tracing here and returns false iff capacity > 4000
    if (capacity > MAX_CAPACITY) {
        return JNI_ERR;
    } else {
        return JNI_OK;
    }
}

jobject jni_AllocObject(JNIEnv *env, jclass clazz) {
    JniUnimplemented("AllocObject");
}

jobject jni_NewObject(JNIEnv *env, jclass clazz, jmethodID id, ...) {
    va_list args;
    va_start(args, id);
    jobject res = CreateJavaObject(clazz);
    CallJavaConstructor(res, id, args);
    va_end(args);
    return res;
}

jobject jni_NewObjectV(JNIEnv *env, jclass clazz, jmethodID id, va_list args) {
    jobject res = CreateJavaObject(clazz);
    CallJavaConstructor(res, id, args);
    return res;
}

jobject jni_NewObjectA(JNIEnv *env, jclass clazz, jmethodID id,
                       const jvalue *args) {
    jobject res = CreateJavaObject(clazz);
    CallJavaNonvirtualMethod<jobject>(res, id, args);
    return res;
}

jclass jni_GetObjectClass(JNIEnv *env, jobject obj) {
    if (obj) {
        return Unwrap(obj)->Cdv()->Class()->Wrap();
    } else {
        return NULL;
    }
}

jboolean jni_IsInstanceOf(JNIEnv *env, jobject obj, jclass clazz) {
    return InstanceOf(obj, clazz);
    // JniUnimplemented("IsInstanceOf");
}

jmethodID jni_GetMethodID(JNIEnv *env, jclass clazz, const char *name,
                          const char *sig) {
    return reinterpret_cast<jmethodID>(
        GetJavaMethodInfo(clazz, name, sig).first);
}

#define ARGS JNIEnv *env, jobject obj, jmethodID id, ...
#define START_VA                                                               \
    va_list args;                                                              \
    va_start(args, id)
#define END_VA va_end(args)
#define CALL(jrep) CallJavaInstanceMethod<jrep>(obj, id, args)
#define IMPL(jrep)                                                             \
    START_VA;                                                                  \
    jrep res = CALL(jrep);                                                     \
    END_VA;                                                                    \
    return res
jobject jni_CallObjectMethod(ARGS) { IMPL(jobject); }
jboolean jni_CallBooleanMethod(ARGS) { IMPL(jboolean); }
jbyte jni_CallByteMethod(ARGS) { IMPL(jbyte); }
jchar jni_CallCharMethod(ARGS) { IMPL(jchar); }
jshort jni_CallShortMethod(ARGS) { IMPL(jshort); }
jint jni_CallIntMethod(ARGS) { IMPL(jint); }
jlong jni_CallLongMethod(ARGS) { IMPL(jlong); }
jfloat jni_CallFloatMethod(ARGS) { IMPL(jfloat); }
jdouble jni_CallDoubleMethod(ARGS) { IMPL(jdouble); }
void jni_CallVoidMethod(ARGS) {
    START_VA;
    CALL(void);
    END_VA;
}
#undef IMPL
#undef CALL
#undef END_VA
#undef START_VA
#undef ARGS

#define ARGS JNIEnv *env, jobject obj, jmethodID id, const jvalue *args
#define IMPL(jrep) return CallJavaInstanceMethod<jrep>(obj, id, args)
jobject jni_CallObjectMethodA(ARGS) { IMPL(jobject); }
jboolean jni_CallBooleanMethodA(ARGS) { IMPL(jboolean); }
jbyte jni_CallByteMethodA(ARGS) { IMPL(jbyte); }
jchar jni_CallCharMethodA(ARGS) { IMPL(jchar); }
jshort jni_CallShortMethodA(ARGS) { IMPL(jshort); }
jint jni_CallIntMethodA(ARGS) { IMPL(jint); }
jlong jni_CallLongMethodA(ARGS) { IMPL(jlong); }
jfloat jni_CallFloatMethodA(ARGS) { IMPL(jfloat); }
jdouble jni_CallDoubleMethodA(ARGS) { IMPL(jdouble); }
void jni_CallVoidMethodA(ARGS) { IMPL(void); }
#undef IMPL
#undef ARGS

#define ARGS JNIEnv *env, jobject obj, jmethodID id, va_list args
#define IMPL(jrep) return CallJavaInstanceMethod<jrep>(obj, id, args)
jobject jni_CallObjectMethodV(ARGS) { IMPL(jobject); }
jboolean jni_CallBooleanMethodV(ARGS) { IMPL(jboolean); }
jbyte jni_CallByteMethodV(ARGS) { IMPL(jbyte); }
jchar jni_CallCharMethodV(ARGS) { IMPL(jchar); }
jshort jni_CallShortMethodV(ARGS) { IMPL(jshort); }
jint jni_CallIntMethodV(ARGS) { IMPL(jint); }
jlong jni_CallLongMethodV(ARGS) { IMPL(jlong); }
jfloat jni_CallFloatMethodV(ARGS) { IMPL(jfloat); }
jdouble jni_CallDoubleMethodV(ARGS) { IMPL(jdouble); }
void jni_CallVoidMethodV(ARGS) { IMPL(void); }
#undef IMPL
#undef ARGS

jobject jni_CallNonvirtualObjectMethod(JNIEnv *env, jobject obj, jclass clazz,
                                       jmethodID id, ...) {
    JniUnimplemented("CallNonvirtualObjectMethod");
}
jboolean jni_CallNonvirtualBooleanMethod(JNIEnv *env, jobject obj, jclass clazz,
                                         jmethodID id, ...) {
    JniUnimplemented("CallNonvirtualBooleanMethod");
}
jbyte jni_CallNonvirtualByteMethod(JNIEnv *env, jobject obj, jclass clazz,
                                   jmethodID id, ...) {
    JniUnimplemented("CallNonvirtualByteMethod");
}
jchar jni_CallNonvirtualCharMethod(JNIEnv *env, jobject obj, jclass clazz,
                                   jmethodID id, ...) {
    JniUnimplemented("CallNonvirtualCharMethod");
}
jshort jni_CallNonvirtualShortMethod(JNIEnv *env, jobject obj, jclass clazz,
                                     jmethodID id, ...) {
    JniUnimplemented("CallNonvirtualShortMethod");
}
jint jni_CallNonvirtualIntMethod(JNIEnv *env, jobject obj, jclass clazz,
                                 jmethodID id, ...) {
    JniUnimplemented("CallNonvirtualIntMethod");
}
jlong jni_CallNonvirtualLongMethod(JNIEnv *env, jobject obj, jclass clazz,
                                   jmethodID id, ...) {
    JniUnimplemented("CallNonvirtualLongMethod");
}
jfloat jni_CallNonvirtualFloatMethod(JNIEnv *env, jobject obj, jclass clazz,
                                     jmethodID id, ...) {
    JniUnimplemented("CallNonvirtualFloatMethod");
}
jdouble jni_CallNonvirtualDoubleMethod(JNIEnv *env, jobject obj, jclass clazz,
                                       jmethodID id, ...) {
    JniUnimplemented("CallNonvirtualDoubleMethod");
}
void jni_CallNonvirtualVoidMethod(JNIEnv *env, jobject obj, jclass clazz,
                                  jmethodID id, ...) {
    JniUnimplemented("CallNonvirtualVoidMethod");
}

jobject jni_CallNonvirtualObjectMethodA(JNIEnv *env, jobject obj, jclass clazz,
                                        jmethodID id, const jvalue *args) {
    JniUnimplemented("CallNonvirtualObjectMethodA");
}
jboolean jni_CallNonvirtualBooleanMethodA(JNIEnv *env, jobject obj,
                                          jclass clazz, jmethodID id,
                                          const jvalue *args) {
    JniUnimplemented("CallNonvirtualBooleanMethodA");
}
jbyte jni_CallNonvirtualByteMethodA(JNIEnv *env, jobject obj, jclass clazz,
                                    jmethodID id, const jvalue *args) {
    JniUnimplemented("CallNonvirtualByteMethodA");
}
jchar jni_CallNonvirtualCharMethodA(JNIEnv *env, jobject obj, jclass clazz,
                                    jmethodID id, const jvalue *args) {
    JniUnimplemented("CallNonvirtualCharMethodA");
}
jshort jni_CallNonvirtualShortMethodA(JNIEnv *env, jobject obj, jclass clazz,
                                      jmethodID id, const jvalue *args) {
    JniUnimplemented("CallNonvirtualShortMethodA");
}
jint jni_CallNonvirtualIntMethodA(JNIEnv *env, jobject obj, jclass clazz,
                                  jmethodID id, const jvalue *args) {
    JniUnimplemented("CallNonvirtualIntMethodA");
}
jlong jni_CallNonvirtualLongMethodA(JNIEnv *env, jobject obj, jclass clazz,
                                    jmethodID id, const jvalue *args) {
    JniUnimplemented("CallNonvirtualLongMethodA");
}
jfloat jni_CallNonvirtualFloatMethodA(JNIEnv *env, jobject obj, jclass clazz,
                                      jmethodID id, const jvalue *args) {
    JniUnimplemented("CallNonvirtualFloatMethodA");
}
jdouble jni_CallNonvirtualDoubleMethodA(JNIEnv *env, jobject obj, jclass clazz,
                                        jmethodID id, const jvalue *args) {
    JniUnimplemented("CallNonvirtualDoubleMethodA");
}
void jni_CallNonvirtualVoidMethodA(JNIEnv *env, jobject obj, jclass clazz,
                                   jmethodID id, const jvalue *args) {
    JniUnimplemented("CallNonvirtualVoidMethodA");
}

jobject jni_CallNonvirtualObjectMethodV(JNIEnv *env, jobject obj, jclass clazz,
                                        jmethodID id, va_list args) {
    JniUnimplemented("CallNonvirtualObjectMethodV");
}
jboolean jni_CallNonvirtualBooleanMethodV(JNIEnv *env, jobject obj,
                                          jclass clazz, jmethodID id,
                                          va_list args) {
    JniUnimplemented("CallNonvirtualBooleanMethodV");
}
jbyte jni_CallNonvirtualByteMethodV(JNIEnv *env, jobject obj, jclass clazz,
                                    jmethodID id, va_list args) {
    JniUnimplemented("CallNonvirtualByteMethodV");
}
jchar jni_CallNonvirtualCharMethodV(JNIEnv *env, jobject obj, jclass clazz,
                                    jmethodID id, va_list args) {
    JniUnimplemented("CallNonvirtualCharMethodV");
}
jshort jni_CallNonvirtualShortMethodV(JNIEnv *env, jobject obj, jclass clazz,
                                      jmethodID id, va_list args) {
    JniUnimplemented("CallNonvirtualShortMethodV");
}
jint jni_CallNonvirtualIntMethodV(JNIEnv *env, jobject obj, jclass clazz,
                                  jmethodID id, va_list args) {
    JniUnimplemented("CallNonvirtualIntMethodV");
}
jlong jni_CallNonvirtualLongMethodV(JNIEnv *env, jobject obj, jclass clazz,
                                    jmethodID id, va_list args) {
    JniUnimplemented("CallNonvirtualLongMethodV");
}
jfloat jni_CallNonvirtualFloatMethodV(JNIEnv *env, jobject obj, jclass clazz,
                                      jmethodID id, va_list args) {
    JniUnimplemented("CallNonvirtualFloatMethodV");
}
jdouble jni_CallNonvirtualDoubleMethodV(JNIEnv *env, jobject obj, jclass clazz,
                                        jmethodID id, va_list args) {
    JniUnimplemented("CallNonvirtualDoubleMethodV");
}
void jni_CallNonvirtualVoidMethodV(JNIEnv *env, jobject obj, jclass clazz,
                                   jmethodID id, va_list args) {
    JniUnimplemented("CallNonvirtualVoidMethodV");
}

jfieldID jni_GetJavaFieldID(JNIEnv *env, jclass clazz, const char *name,
                            const char *sig) {
    // Scare casts to wrap field info in opaque pointer.
    auto field = GetJavaFieldInfo(clazz, name);
    auto no_const = const_cast<JavaFieldInfo *>(field);
    return reinterpret_cast<jfieldID>(no_const);
}

// TODO: Ideally we would do more error checking here,
//       to protect against buggy native code.
#define ARGS JNIEnv *env, jobject obj, jfieldID id
#define IMPL(jrep) return GetJavaField<jrep>(obj, id)
jboolean jni_GetBooleanField(ARGS) { IMPL(jboolean); }
jbyte jni_GetByteField(ARGS) { IMPL(jbyte); }
jchar jni_GetCharField(ARGS) { IMPL(jchar); }
jshort jni_GetShortField(ARGS) { IMPL(jshort); }
jint jni_GetIntField(ARGS) { IMPL(jint); }
jlong jni_GetLongField(ARGS) { IMPL(jlong); }
jfloat jni_GetFloatField(ARGS) { IMPL(jfloat); }
jdouble jni_GetDoubleField(ARGS) { IMPL(jdouble); }
jobject jni_GetObjectField(ARGS) { IMPL(jobject); }
#undef IMPL
#undef ARGS

#define ARGS(jrep) JNIEnv *env, jobject obj, jfieldID id, jrep val
#define IMPL(jrep) return SetJavaField<jrep>(obj, id, val)
void jni_SetBooleanField(ARGS(jboolean)) { IMPL(jboolean); }
void jni_SetByteField(ARGS(jbyte)) { IMPL(jbyte); }
void jni_SetCharField(ARGS(jchar)) { IMPL(jchar); }
void jni_SetShortField(ARGS(jshort)) { IMPL(jshort); }
void jni_SetIntField(ARGS(jint)) { IMPL(jint); }
void jni_SetLongField(ARGS(jlong)) { IMPL(jlong); }
void jni_SetFloatField(ARGS(jfloat)) { IMPL(jfloat); }
void jni_SetDoubleField(ARGS(jdouble)) { IMPL(jdouble); }
void jni_SetObjectField(ARGS(jobject)) { IMPL(jobject); }
#undef IMPL
#undef ARGS

jmethodID jni_GetStaticMethodID(JNIEnv *env, jclass clazz, const char *name,
                                const char *sig) {
    return reinterpret_cast<jmethodID>(
        GetJavaStaticMethodInfo(clazz, name, sig).first);
}

#define ARGS JNIEnv *env, jclass cls, jmethodID id, ...
#define START_VA                                                               \
    va_list args;                                                              \
    va_start(args, id)
#define END_VA va_end(args)
#define CALL(jrep) CallJavaStaticMethod<jrep>(cls, id, args)
#define IMPL(jrep)                                                             \
    START_VA;                                                                  \
    jrep res = CALL(jrep);                                                     \
    END_VA;                                                                    \
    return res
jobject jni_CallStaticObjectMethod(ARGS) { IMPL(jobject); }
jboolean jni_CallStaticBooleanMethod(ARGS) { IMPL(jboolean); }
jbyte jni_CallStaticByteMethod(ARGS) { IMPL(jbyte); }
jchar jni_CallStaticCharMethod(ARGS) { IMPL(jchar); }
jshort jni_CallStaticShortMethod(ARGS) { IMPL(jshort); }
jint jni_CallStaticIntMethod(ARGS) { IMPL(jint); }
jlong jni_CallStaticLongMethod(ARGS) { IMPL(jlong); }
jfloat jni_CallStaticFloatMethod(ARGS) { IMPL(jfloat); }
jdouble jni_CallStaticDoubleMethod(ARGS) { IMPL(jdouble); }
void jni_CallStaticVoidMethod(ARGS) {
    START_VA;
    CALL(void);
    END_VA;
}
#undef IMPL
#undef CALL
#undef END_VA
#undef START_VA
#undef ARGS

#define ARGS JNIEnv *env, jclass cls, jmethodID id, va_list args
#define IMPL(jrep) return CallJavaStaticMethod<jrep>(cls, id, args)
jobject jni_CallStaticObjectMethodV(ARGS) { IMPL(jobject); }
jboolean jni_CallStaticBooleanMethodV(ARGS) { IMPL(jboolean); }
jbyte jni_CallStaticByteMethodV(ARGS) { IMPL(jbyte); }
jchar jni_CallStaticCharMethodV(ARGS) { IMPL(jchar); }
jshort jni_CallStaticShortMethodV(ARGS) { IMPL(jshort); }
jint jni_CallStaticIntMethodV(ARGS) { IMPL(jint); }
jlong jni_CallStaticLongMethodV(ARGS) { IMPL(jlong); }
jfloat jni_CallStaticFloatMethodV(ARGS) { IMPL(jfloat); }
jdouble jni_CallStaticDoubleMethodV(ARGS) { IMPL(jdouble); }
void jni_CallStaticVoidMethodV(ARGS) {
    CallJavaStaticMethod<void>(cls, id, args);
}
#undef IMPL
#undef ARGS

jobject jni_CallStaticObjectMethodA(JNIEnv *env, jclass cls, jmethodID id,
                                    const jvalue *args) {
    JniUnimplemented("CallStaticObjectMethodA");
}
jboolean jni_CallStaticBooleanMethodA(JNIEnv *env, jclass cls, jmethodID id,
                                      const jvalue *args) {
    JniUnimplemented("CallStaticBooleanMethodA");
}
jbyte jni_CallStaticByteMethodA(JNIEnv *env, jclass cls, jmethodID id,
                                const jvalue *args) {
    JniUnimplemented("CallStaticByteMethodA");
}
jchar jni_CallStaticCharMethodA(JNIEnv *env, jclass cls, jmethodID id,
                                const jvalue *args) {
    JniUnimplemented("CallStaticCharMethodA");
}
jshort jni_CallStaticShortMethodA(JNIEnv *env, jclass cls, jmethodID id,
                                  const jvalue *args) {
    JniUnimplemented("CallStaticShortMethodA");
}
jint jni_CallStaticIntMethodA(JNIEnv *env, jclass cls, jmethodID id,
                              const jvalue *args) {
    JniUnimplemented("CallStaticIntMethodA");
}
jlong jni_CallStaticLongMethodA(JNIEnv *env, jclass cls, jmethodID id,
                                const jvalue *args) {
    JniUnimplemented("CallStaticLongMethodA");
}
jfloat jni_CallStaticFloatMethodA(JNIEnv *env, jclass cls, jmethodID id,
                                  const jvalue *args) {
    JniUnimplemented("CallStaticFloatMethodA");
}
jdouble jni_CallStaticDoubleMethodA(JNIEnv *env, jclass cls, jmethodID id,
                                    const jvalue *args) {
    JniUnimplemented("CallStaticDoubleMethodA");
}
void jni_CallStaticVoidMethodA(JNIEnv *env, jclass cls, jmethodID id,
                               const jvalue *args) {
    JniUnimplemented("CallStaticVoidMethodA");
}

jfieldID jni_GetStaticFieldID(JNIEnv *env, jclass clazz, const char *name,
                              const char *sig) {
    auto field = GetJavaStaticFieldInfo(clazz, name, sig);
    auto no_const = const_cast<JavaStaticFieldInfo *>(field);
    return reinterpret_cast<jfieldID>(no_const);
}

jobject jni_GetStaticObjectField(JNIEnv *env, jclass clazz, jfieldID id) {
    JniUnimplemented("GetStaticObjectField");
}
jboolean jni_GetStaticBooleanField(JNIEnv *env, jclass clazz, jfieldID id) {
    JniUnimplemented("GetStaticBooleanField");
}
jbyte jni_GetStaticByteField(JNIEnv *env, jclass clazz, jfieldID id) {
    JniUnimplemented("GetStaticByteField");
}
jchar jni_GetStaticCharField(JNIEnv *env, jclass clazz, jfieldID id) {
    JniUnimplemented("GetStaticCharField");
}
jshort jni_GetStaticShortField(JNIEnv *env, jclass clazz, jfieldID id) {
    JniUnimplemented("GetStaticShortField");
}
jint jni_GetStaticIntField(JNIEnv *env, jclass clazz, jfieldID id) {
    JniUnimplemented("GetStaticIntField");
}
jlong jni_GetStaticLongField(JNIEnv *env, jclass clazz, jfieldID id) {
    JniUnimplemented("GetStaticLongField");
}
jfloat jni_GetStaticFloatField(JNIEnv *env, jclass clazz, jfieldID id) {
    JniUnimplemented("GetStaticFloatField");
}
jdouble jni_GetStaticDoubleField(JNIEnv *env, jclass clazz, jfieldID id) {
    JniUnimplemented("GetStaticDoubleField");
}

#define ARGS(jrep) JNIEnv *env, jclass clazz, jfieldID id, jrep val
#define IMPL(jrep) return SetJavaStaticField<jrep>(clazz, id, val)
void jni_SetStaticObjectField(ARGS(jobject)) { IMPL(jobject); }
void jni_SetStaticBooleanField(ARGS(jboolean)) { IMPL(jboolean); }
void jni_SetStaticByteField(ARGS(jbyte)) { IMPL(jbyte); }
void jni_SetStaticCharField(ARGS(jchar)) { IMPL(jchar); }
void jni_SetStaticShortField(ARGS(jshort)) { IMPL(jshort); }
void jni_SetStaticIntField(ARGS(jint)) { IMPL(jint); }
void jni_SetStaticLongField(ARGS(jlong)) { IMPL(jlong); }
void jni_SetStaticFloatField(ARGS(jfloat)) { IMPL(jfloat); }
void jni_SetStaticDoubleField(ARGS(jdouble)) { IMPL(jdouble); }
#undef IMPL
#undef ARGS

jstring jni_NewString(JNIEnv *env, const jchar *unicode, jsize len) {
    return CreateJavaString(unicode, len);
}

jsize jni_GetStringLength(JNIEnv *env, jstring str) {
    return Unwrap(str)->Chars()->Length();
}

const jchar *jni_GetStringChars(JNIEnv *env, jstring str, jboolean *isCopy) {
    auto chars = reinterpret_cast<jcharArray>(Unwrap(str)->Chars()->Wrap());
    return env->GetCharArrayElements(chars, isCopy);
}

void jni_ReleaseStringChars(JNIEnv *env, jstring str, const jchar *elems) {
    auto chars = reinterpret_cast<jcharArray>(Unwrap(str)->Chars()->Wrap());
    auto elems_cast = const_cast<jchar *>(elems);
    env->ReleaseCharArrayElements(chars, elems_cast, /*mode*/ 0);
}

jstring jni_NewStringUTF(JNIEnv *env, const char *utf) {
    jsize len = strlen(utf);
    jcharArray chars = env->NewCharArray(len);
    jchar *data = env->GetCharArrayElements(chars, /*isCopy*/ nullptr);
    // TODO: Not a proper character encoding conversion.
    for (jsize i = 0; i < len; ++i)
        data[i] = static_cast<jchar>(utf[i]);
    env->ReleaseCharArrayElements(chars, data, /*mode*/ 0);
    jstring res = CreateJavaString(chars);
    return res;
}

jsize jni_GetStringUTFLength(JNIEnv *env, jstring str) {
    auto chars = env->GetStringUTFChars(str, /*isCopy*/ nullptr);
    jsize len = strlen(chars);
    env->ReleaseStringUTFChars(str, chars);
    return len;
}

const char *jni_GetStringUTFChars(JNIEnv *env, jstring str, jboolean *isCopy) {
    auto len = env->GetStringLength(str);
    auto chars = env->GetStringChars(str, /*isCopy*/ nullptr);
    char *res = (char *)malloc(len + 1);
    // TODO: Incorrect conversion from Java string chars to UTF-8 chars.
    // only works for 1 byte long UTF-16 chars (a.k.a. things that are already
    // in UTF-8 form)
    for (jsize i = 0; i < len; ++i)
        res[i] = static_cast<char>(chars[i]);
    res[len] = '\0';
    env->ReleaseStringChars(str, chars);
    if (isCopy != nullptr)
        *isCopy = true;
    return res;
}

void jni_ReleaseStringUTFChars(JNIEnv *env, jstring str, const char *chars) {
    free(const_cast<char *>(chars));
}

jsize jni_GetArrayLength(JNIEnv *env, jarray array) {
    return Unwrap(array)->Length();
}

jobjectArray jni_NewObjectArray(JNIEnv *env, jsize len, jclass clazz,
                                jobject init) {
    JniUnimplemented("NewObjectArray");
}

jobject jni_GetObjectArrayElement(JNIEnv *env, jobjectArray arr, jsize index) {
    if (!JavaArrayBoundsCheck(arr, index))
        return nullptr;
    // Assumes no copy.
    auto data = GetJavaArrayData<jobject>(arr, /*isCopy*/ nullptr);
    return data[index];
}

void jni_SetObjectArrayElement(JNIEnv *env, jobjectArray arr, jsize index,
                               jobject val) {
    if (!JavaArrayBoundsCheck(arr, index))
        return;
    // Assumes no copy.
    auto data = GetJavaArrayData<jobject>(arr, /*isCopy*/ nullptr);
    data[index] = val;
}

#define ARGS JNIEnv *env, jsize len
jbooleanArray jni_NewBooleanArray(ARGS) { return CreateJavaBooleanArray(len); }
jbyteArray jni_NewByteArray(ARGS) { return CreateJavaByteArray(len); }
jcharArray jni_NewCharArray(ARGS) { return CreateJavaCharArray(len); }
jshortArray jni_NewShortArray(ARGS) { return CreateJavaShortArray(len); }
jintArray jni_NewIntArray(ARGS) { return CreateJavaIntArray(len); }
jlongArray jni_NewLongArray(ARGS) { return CreateJavaLongArray(len); }
jfloatArray jni_NewFloatArray(ARGS) { return CreateJavaFloatArray(len); }
jdoubleArray jni_NewDoubleArray(ARGS) { return CreateJavaDoubleArray(len); }
#undef ARGS

jboolean *jni_GetBooleanArrayElements(JNIEnv *env, jbooleanArray arr,
                                      jboolean *isCopy) {
    return GetJavaArrayData<jboolean>(arr, isCopy);
}
jbyte *jni_GetByteArrayElements(JNIEnv *env, jbyteArray arr, jboolean *isCopy) {
    return GetJavaArrayData<jbyte>(arr, isCopy);
}
jchar *jni_GetCharArrayElements(JNIEnv *env, jcharArray arr, jboolean *isCopy) {
    return GetJavaArrayData<jchar>(arr, isCopy);
}
jshort *jni_GetShortArrayElements(JNIEnv *env, jshortArray arr,
                                  jboolean *isCopy) {
    return GetJavaArrayData<jshort>(arr, isCopy);
}
jint *jni_GetIntArrayElements(JNIEnv *env, jintArray arr, jboolean *isCopy) {
    return GetJavaArrayData<jint>(arr, isCopy);
}
jlong *jni_GetLongArrayElements(JNIEnv *env, jlongArray arr, jboolean *isCopy) {
    return GetJavaArrayData<jlong>(arr, isCopy);
}
jfloat *jni_GetFloatArrayElements(JNIEnv *env, jfloatArray arr,
                                  jboolean *isCopy) {
    return GetJavaArrayData<jfloat>(arr, isCopy);
}
jdouble *jni_GetDoubleArrayElements(JNIEnv *env, jdoubleArray arr,
                                    jboolean *isCopy) {
    return GetJavaArrayData<jdouble>(arr, isCopy);
}

// We do not make a copy of array elements, so these are no-ops.
void jni_ReleaseBooleanArrayElements(JNIEnv *env, jbooleanArray arr,
                                     jboolean *data, jint mode) {}
void jni_ReleaseByteArrayElements(JNIEnv *env, jbyteArray arr, jbyte *data,
                                  jint mode) {}
void jni_ReleaseCharArrayElements(JNIEnv *env, jcharArray arr, jchar *data,
                                  jint mode) {}
void jni_ReleaseShortArrayElements(JNIEnv *env, jshortArray arr, jshort *data,
                                   jint mode) {}
void jni_ReleaseIntArrayElements(JNIEnv *env, jintArray arr, jint *data,
                                 jint mode) {}
void jni_ReleaseLongArrayElements(JNIEnv *env, jlongArray arr, jlong *data,
                                  jint mode) {}
void jni_ReleaseFloatArrayElements(JNIEnv *env, jfloatArray arr, jfloat *data,
                                   jint mode) {}
void jni_ReleaseDoubleArrayElements(JNIEnv *env, jdoubleArray arr,
                                    jdouble *data, jint mode) {}

void jni_GetBooleanArrayRegion(JNIEnv *env, jbooleanArray arr, jsize start,
                               jsize len, jboolean *buf) {
    GetJavaArrayRegion<jboolean>(arr, start, len, buf);
}
void jni_GetByteArrayRegion(JNIEnv *env, jbyteArray arr, jsize start, jsize len,
                            jbyte *buf) {
    GetJavaArrayRegion<jbyte>(arr, start, len, buf);
}
void jni_GetCharArrayRegion(JNIEnv *env, jcharArray arr, jsize start, jsize len,
                            jchar *buf) {
    GetJavaArrayRegion<jchar>(arr, start, len, buf);
}
void jni_GetShortArrayRegion(JNIEnv *env, jshortArray arr, jsize start,
                             jsize len, jshort *buf) {
    GetJavaArrayRegion<jshort>(arr, start, len, buf);
}
void jni_GetIntArrayRegion(JNIEnv *env, jintArray arr, jsize start, jsize len,
                           jint *buf) {
    GetJavaArrayRegion<jint>(arr, start, len, buf);
}
void jni_GetLongArrayRegion(JNIEnv *env, jlongArray arr, jsize start, jsize len,
                            jlong *buf) {
    GetJavaArrayRegion<jlong>(arr, start, len, buf);
}
void jni_GetFloatArrayRegion(JNIEnv *env, jfloatArray arr, jsize start,
                             jsize len, jfloat *buf) {
    GetJavaArrayRegion<jfloat>(arr, start, len, buf);
}
void jni_GetDoubleArrayRegion(JNIEnv *env, jdoubleArray arr, jsize start,
                              jsize len, jdouble *buf) {
    GetJavaArrayRegion<jdouble>(arr, start, len, buf);
}

void jni_SetBooleanArrayRegion(JNIEnv *env, jbooleanArray arr, jsize start,
                               jsize len, const jboolean *buf) {
    SetJavaArrayRegion<jboolean>(arr, start, len, buf);
}
void jni_SetByteArrayRegion(JNIEnv *env, jbyteArray arr, jsize start, jsize len,
                            const jbyte *buf) {
    SetJavaArrayRegion<jbyte>(arr, start, len, buf);
}
void jni_SetCharArrayRegion(JNIEnv *env, jcharArray arr, jsize start, jsize len,
                            const jchar *buf) {
    SetJavaArrayRegion<jchar>(arr, start, len, buf);
}
void jni_SetShortArrayRegion(JNIEnv *env, jshortArray arr, jsize start,
                             jsize len, const jshort *buf) {
    SetJavaArrayRegion<jshort>(arr, start, len, buf);
}
void jni_SetIntArrayRegion(JNIEnv *env, jintArray arr, jsize start, jsize len,
                           const jint *buf) {
    SetJavaArrayRegion<jint>(arr, start, len, buf);
}
void jni_SetLongArrayRegion(JNIEnv *env, jlongArray arr, jsize start, jsize len,
                            const jlong *buf) {
    SetJavaArrayRegion<jlong>(arr, start, len, buf);
}
void jni_SetFloatArrayRegion(JNIEnv *env, jfloatArray arr, jsize start,
                             jsize len, const jfloat *buf) {
    SetJavaArrayRegion<jfloat>(arr, start, len, buf);
}
void jni_SetDoubleArrayRegion(JNIEnv *env, jdoubleArray arr, jsize start,
                              jsize len, const jdouble *buf) {
    SetJavaArrayRegion<jdouble>(arr, start, len, buf);
}

jint jni_RegisterNatives(JNIEnv *env, jclass clazz,
                         const JNINativeMethod *methods, jint nMethods) {
    for (jint i = 0; i < nMethods; ++i) {
        RegisterJavaNativeFunc(clazz, methods[i].name, methods[i].signature,
                               methods[i].fnPtr);
    }
    return 0;
}

jint jni_UnregisterNatives(JNIEnv *env, jclass clazz) {
    JniUnimplemented("UnregisterNatives");
}

jint jni_MonitorEnter(JNIEnv *env, jobject obj) {
    printf("MonitorEnter: %p\n", (void*)obj);
    return 0;
}

jint jni_MonitorExit(JNIEnv *env, jobject obj) {
    printf("MonitorExit: %p\n", (void*)obj);
    return 0;
}

jint jni_GetJavaVM(JNIEnv *env, JavaVM **vm);

void jni_GetStringRegion(JNIEnv *env, jstring str, jsize start, jsize len,
                         jchar *buf) {
    // TODO error handling on case: if (start < 0 || len <0 || start + len >
    // s_len) {  and throw StringIndexOutOfBoundsException
    JArrayRep *str_array = Unwrap(str)->Chars();
    // int str_len = str_array->Length();
    int elemsize = str_array->ElemSize();
    assert(elemsize == sizeof(jchar));
    jchar *data = reinterpret_cast<jchar *>(str_array->Data());
    memcpy(buf, &(data[start]), sizeof(jchar) * len);
}

void jni_GetStringUTFRegion(JNIEnv *env, jstring str, jsize start, jsize len,
                            char *buf) {
    // TODO error handling on case: if (start < 0 || len < 0 || start + len >
    // s_len) {  and throw StringIndexOutOfBoundsException
    if (len > 0) {
        JArrayRep *str_array = Unwrap(str)->Chars();
        // int str_len = str_array->Length();
        int elemsize = str_array->ElemSize();
        char *str_data = (char *)str_array->Data();
        as_utf8((jchar *)(str_data + (elemsize * start)), len, (u_char *)buf);
        int utf_len = (int)strlen(buf);
        buf[utf_len] = 0;
    } else {
        if (buf != NULL) {
            buf[0] = 0;
        }
    }
}

void *jni_GetPrimitiveArrayCritical(JNIEnv *env, jarray array,
                                    jboolean *isCopy) {
    JniUnimplemented("GetPrimitiveArrayCritical");
}

void jni_ReleasePrimitiveArrayCritical(JNIEnv *env, jarray array, void *carray,
                                       jint mode) {
    JniUnimplemented("ReleasePrimitiveArrayCritical");
}

const jchar *jni_GetStringCritical(JNIEnv *env, jstring string,
                                   jboolean *isCopy) {
    JniUnimplemented("GetStringCritical");
}

void jni_ReleaseStringCritical(JNIEnv *env, jstring string,
                               const jchar *cstring) {
    JniUnimplemented("ReleaseStringCritical");
}

jweak jni_NewWeakGlobalRef(JNIEnv *env, jobject obj) {
    JniUnimplemented("NewWeakGlobalRef");
}

void jni_DeleteWeakGlobalRef(JNIEnv *env, jweak ref) {
    JniUnimplemented("DeleteWeakGlobalRef");
}

jboolean jni_ExceptionCheck(JNIEnv *env) {
    // TODO implement pending exceptions
    return JNI_FALSE;
}

jobject jni_NewDirectByteBuffer(JNIEnv *env, void *address, jlong capacity) {
    JniUnimplemented("NewDirectByteBuffer");
}

void *jni_GetDirectBufferAddress(JNIEnv *env, jobject buf) {
    JniUnimplemented("GetDirectBufferAddress");
}

jlong jni_GetDirectBufferCapacity(JNIEnv *env, jobject buf) {
    JniUnimplemented("GetDirectBufferCapacity");
}

jobjectRefType jni_GetObjectRefType(JNIEnv *env, jobject obj) {
    JniUnimplemented("GetObjectRefType");
}

const struct JNINativeInterface_ jni_NativeInterface = {
    NULL,
    NULL,
    NULL,

    NULL,

    jni_GetVersion,

    jni_DefineClass,
    jni_FindClass,

    jni_FromReflectedMethod,
    jni_FromReflectedField,

    jni_ToReflectedMethod,

    jni_GetSuperclass,
    jni_IsAssignableFrom,

    jni_ToReflectedField,

    jni_Throw,
    jni_ThrowNew,
    jni_ExceptionOccurred,
    jni_ExceptionDescribe,
    jni_ExceptionClear,
    jni_FatalError,

    jni_PushLocalFrame,
    jni_PopLocalFrame,

    jni_NewGlobalRef,
    jni_DeleteGlobalRef,
    jni_DeleteLocalRef,
    jni_IsSameObject,

    jni_NewLocalRef,
    jni_EnsureLocalCapacity,

    jni_AllocObject,
    jni_NewObject,
    jni_NewObjectV,
    jni_NewObjectA,

    jni_GetObjectClass,
    jni_IsInstanceOf,

    jni_GetMethodID,

    jni_CallObjectMethod,
    jni_CallObjectMethodV,
    jni_CallObjectMethodA,
    jni_CallBooleanMethod,
    jni_CallBooleanMethodV,
    jni_CallBooleanMethodA,
    jni_CallByteMethod,
    jni_CallByteMethodV,
    jni_CallByteMethodA,
    jni_CallCharMethod,
    jni_CallCharMethodV,
    jni_CallCharMethodA,
    jni_CallShortMethod,
    jni_CallShortMethodV,
    jni_CallShortMethodA,
    jni_CallIntMethod,
    jni_CallIntMethodV,
    jni_CallIntMethodA,
    jni_CallLongMethod,
    jni_CallLongMethodV,
    jni_CallLongMethodA,
    jni_CallFloatMethod,
    jni_CallFloatMethodV,
    jni_CallFloatMethodA,
    jni_CallDoubleMethod,
    jni_CallDoubleMethodV,
    jni_CallDoubleMethodA,
    jni_CallVoidMethod,
    jni_CallVoidMethodV,
    jni_CallVoidMethodA,

    jni_CallNonvirtualObjectMethod,
    jni_CallNonvirtualObjectMethodV,
    jni_CallNonvirtualObjectMethodA,
    jni_CallNonvirtualBooleanMethod,
    jni_CallNonvirtualBooleanMethodV,
    jni_CallNonvirtualBooleanMethodA,
    jni_CallNonvirtualByteMethod,
    jni_CallNonvirtualByteMethodV,
    jni_CallNonvirtualByteMethodA,
    jni_CallNonvirtualCharMethod,
    jni_CallNonvirtualCharMethodV,
    jni_CallNonvirtualCharMethodA,
    jni_CallNonvirtualShortMethod,
    jni_CallNonvirtualShortMethodV,
    jni_CallNonvirtualShortMethodA,
    jni_CallNonvirtualIntMethod,
    jni_CallNonvirtualIntMethodV,
    jni_CallNonvirtualIntMethodA,
    jni_CallNonvirtualLongMethod,
    jni_CallNonvirtualLongMethodV,
    jni_CallNonvirtualLongMethodA,
    jni_CallNonvirtualFloatMethod,
    jni_CallNonvirtualFloatMethodV,
    jni_CallNonvirtualFloatMethodA,
    jni_CallNonvirtualDoubleMethod,
    jni_CallNonvirtualDoubleMethodV,
    jni_CallNonvirtualDoubleMethodA,
    jni_CallNonvirtualVoidMethod,
    jni_CallNonvirtualVoidMethodV,
    jni_CallNonvirtualVoidMethodA,

    jni_GetJavaFieldID,

    jni_GetObjectField,
    jni_GetBooleanField,
    jni_GetByteField,
    jni_GetCharField,
    jni_GetShortField,
    jni_GetIntField,
    jni_GetLongField,
    jni_GetFloatField,
    jni_GetDoubleField,

    jni_SetObjectField,
    jni_SetBooleanField,
    jni_SetByteField,
    jni_SetCharField,
    jni_SetShortField,
    jni_SetIntField,
    jni_SetLongField,
    jni_SetFloatField,
    jni_SetDoubleField,

    jni_GetStaticMethodID,

    jni_CallStaticObjectMethod,
    jni_CallStaticObjectMethodV,
    jni_CallStaticObjectMethodA,
    jni_CallStaticBooleanMethod,
    jni_CallStaticBooleanMethodV,
    jni_CallStaticBooleanMethodA,
    jni_CallStaticByteMethod,
    jni_CallStaticByteMethodV,
    jni_CallStaticByteMethodA,
    jni_CallStaticCharMethod,
    jni_CallStaticCharMethodV,
    jni_CallStaticCharMethodA,
    jni_CallStaticShortMethod,
    jni_CallStaticShortMethodV,
    jni_CallStaticShortMethodA,
    jni_CallStaticIntMethod,
    jni_CallStaticIntMethodV,
    jni_CallStaticIntMethodA,
    jni_CallStaticLongMethod,
    jni_CallStaticLongMethodV,
    jni_CallStaticLongMethodA,
    jni_CallStaticFloatMethod,
    jni_CallStaticFloatMethodV,
    jni_CallStaticFloatMethodA,
    jni_CallStaticDoubleMethod,
    jni_CallStaticDoubleMethodV,
    jni_CallStaticDoubleMethodA,
    jni_CallStaticVoidMethod,
    jni_CallStaticVoidMethodV,
    jni_CallStaticVoidMethodA,

    jni_GetStaticFieldID,

    jni_GetStaticObjectField,
    jni_GetStaticBooleanField,
    jni_GetStaticByteField,
    jni_GetStaticCharField,
    jni_GetStaticShortField,
    jni_GetStaticIntField,
    jni_GetStaticLongField,
    jni_GetStaticFloatField,
    jni_GetStaticDoubleField,

    jni_SetStaticObjectField,
    jni_SetStaticBooleanField,
    jni_SetStaticByteField,
    jni_SetStaticCharField,
    jni_SetStaticShortField,
    jni_SetStaticIntField,
    jni_SetStaticLongField,
    jni_SetStaticFloatField,
    jni_SetStaticDoubleField,

    jni_NewString,
    jni_GetStringLength,
    jni_GetStringChars,
    jni_ReleaseStringChars,

    jni_NewStringUTF,
    jni_GetStringUTFLength,
    jni_GetStringUTFChars,
    jni_ReleaseStringUTFChars,

    jni_GetArrayLength,

    jni_NewObjectArray,
    jni_GetObjectArrayElement,
    jni_SetObjectArrayElement,

    jni_NewBooleanArray,
    jni_NewByteArray,
    jni_NewCharArray,
    jni_NewShortArray,
    jni_NewIntArray,
    jni_NewLongArray,
    jni_NewFloatArray,
    jni_NewDoubleArray,

    jni_GetBooleanArrayElements,
    jni_GetByteArrayElements,
    jni_GetCharArrayElements,
    jni_GetShortArrayElements,
    jni_GetIntArrayElements,
    jni_GetLongArrayElements,
    jni_GetFloatArrayElements,
    jni_GetDoubleArrayElements,

    jni_ReleaseBooleanArrayElements,
    jni_ReleaseByteArrayElements,
    jni_ReleaseCharArrayElements,
    jni_ReleaseShortArrayElements,
    jni_ReleaseIntArrayElements,
    jni_ReleaseLongArrayElements,
    jni_ReleaseFloatArrayElements,
    jni_ReleaseDoubleArrayElements,

    jni_GetBooleanArrayRegion,
    jni_GetByteArrayRegion,
    jni_GetCharArrayRegion,
    jni_GetShortArrayRegion,
    jni_GetIntArrayRegion,
    jni_GetLongArrayRegion,
    jni_GetFloatArrayRegion,
    jni_GetDoubleArrayRegion,

    jni_SetBooleanArrayRegion,
    jni_SetByteArrayRegion,
    jni_SetCharArrayRegion,
    jni_SetShortArrayRegion,
    jni_SetIntArrayRegion,
    jni_SetLongArrayRegion,
    jni_SetFloatArrayRegion,
    jni_SetDoubleArrayRegion,

    jni_RegisterNatives,
    jni_UnregisterNatives,

    jni_MonitorEnter,
    jni_MonitorExit,

    jni_GetJavaVM,

    jni_GetStringRegion,
    jni_GetStringUTFRegion,

    jni_GetPrimitiveArrayCritical,
    jni_ReleasePrimitiveArrayCritical,

    jni_GetStringCritical,
    jni_ReleaseStringCritical,

    jni_NewWeakGlobalRef,
    jni_DeleteWeakGlobalRef,

    jni_ExceptionCheck,

    jni_NewDirectByteBuffer,
    jni_GetDirectBufferAddress,
    jni_GetDirectBufferCapacity,

    jni_GetObjectRefType};

extern const JNIEnv jni_JNIEnv = {&jni_NativeInterface};

jint jni_DestroyJavaVM(JavaVM *vm) { JniUnimplemented("DestroyJavaVM"); }

jint jni_AttachCurrentThread(JavaVM *vm, void **penv, void *args) {
    JniUnimplemented("AttachCurrentThread");
}

jint jni_DetachCurrentThread(JavaVM *vm) {
    JniUnimplemented("DetachCurrentThread");
}

jint jni_GetEnv(JavaVM *vm, void **penv, jint version) {
    *penv = (void *)&jni_JNIEnv;
    return 0; // TODO handle errors / version checking
}

jint jni_AttachCurrentThreadAsDaemon(JavaVM *vm, void **penv, void *args) {
    JniUnimplemented("AttachCurrentThreadAsDaemon");
}

const struct JNIInvokeInterface_ jni_InvokeInterface = {
    NULL,
    NULL,
    NULL,

    jni_DestroyJavaVM,
    jni_AttachCurrentThread,
    jni_DetachCurrentThread,
    jni_GetEnv,
    jni_AttachCurrentThreadAsDaemon};

extern const JavaVM jni_MainJavaVM = {&jni_InvokeInterface};

jint jni_GetJavaVM(JNIEnv *env, JavaVM **vm) {
    *vm = const_cast<JavaVM *>(&jni_MainJavaVM);
    return 0; // TODO handle errors and multi threading
}

} // extern "C"
