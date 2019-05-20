// Copyright (C) 2018 Cornell University

#include "jni.h"
#include <string.h>

#include "class.h"
#include "factory.h"
#include "gc.h"
#include "rep.h"

#define ARRAY_CLS "jlang.runtime.Array"
// The name must match that used in jlang.runtime.Factory,
// and the mangling and calling conventions must match those used by JLang.
extern "C" {

jbooleanArray Polyglot_jlang_runtime_Factory_BooleanArray__I(jint);
jbyteArray Polyglot_jlang_runtime_Factory_ByteArray__I(jint);
jcharArray Polyglot_jlang_runtime_Factory_CharArray__I(jint);
jshortArray Polyglot_jlang_runtime_Factory_ShortArray__I(jint);
jintArray Polyglot_jlang_runtime_Factory_IntArray__I(jint);
jlongArray Polyglot_jlang_runtime_Factory_LongArray__I(jint);
jfloatArray Polyglot_jlang_runtime_Factory_FloatArray__I(jint);
jdoubleArray Polyglot_jlang_runtime_Factory_DoubleArray__I(jint);
jobjectArray Polyglot_jlang_runtime_Factory_ObjectArray__I(jint);

jstring Polyglot_jlang_runtime_Factory_String___3C(jarray);

} // extern "C"

jbooleanArray CreateJavaBooleanArray(jint len) {
    return Polyglot_jlang_runtime_Factory_BooleanArray__I(len);
}
jbyteArray CreateJavaByteArray(jint len) {
    return Polyglot_jlang_runtime_Factory_ByteArray__I(len);
}
jcharArray CreateJavaCharArray(jint len) {
    return Polyglot_jlang_runtime_Factory_CharArray__I(len);
}
jshortArray CreateJavaShortArray(jint len) {
    return Polyglot_jlang_runtime_Factory_ShortArray__I(len);
}
jintArray CreateJavaIntArray(jint len) {
    return Polyglot_jlang_runtime_Factory_IntArray__I(len);
}
jlongArray CreateJavaLongArray(jint len) {
    return Polyglot_jlang_runtime_Factory_LongArray__I(len);
}
jfloatArray CreateJavaFloatArray(jint len) {
    return Polyglot_jlang_runtime_Factory_FloatArray__I(len);
}
jdoubleArray CreateJavaDoubleArray(jint len) {
    return Polyglot_jlang_runtime_Factory_DoubleArray__I(len);
}
jobjectArray CreateJavaObjectArray(jint len) {
    return Polyglot_jlang_runtime_Factory_ObjectArray__I(len);
}

jstring CreateJavaString(jcharArray chars) {
    return Polyglot_jlang_runtime_Factory_String___3C(chars);
}

jobject CreateJavaObject(jclass clazz) {
    auto info = GetJavaClassInfo(clazz);
    // TODO set exception (class is interface or abstract)
    if (info == NULL || info->cdv == NULL) {
        return NULL;
    }
    JObjectRep *new_obj = (JObjectRep *)GC_MALLOC(info->obj_size);
    if (new_obj == NULL) {
        return NULL;
    }
    new_obj->SetCdv(reinterpret_cast<DispatchVector *>(info->cdv));
    return new_obj->Wrap();
}

jobject CloneJavaObject(jobject obj) {
    // TODO set exception if class is not cloneable
    auto objRep = Unwrap(obj);
    auto cdv = objRep->Cdv();
    auto cls = cdv->Class()->Wrap();
    auto info = GetJavaClassInfo(cls);
    int size = 0;
    jobject new_obj;
    if (isArrayClass(cls) || strcmp(info->name, ARRAY_CLS) == 0) {
        JArrayRep *array = Unwrap(reinterpret_cast<jarray>(obj));
        // info-> obj_size == sizeof(JArrayRep)
        size = info->obj_size + (array->Length() * array->ElemSize());
        new_obj = (jobject)GC_MALLOC(size);
    } else {
        new_obj = CreateJavaObject(cls);
        size = info->obj_size;
    }
    memcpy(new_obj, obj, size);
    return new_obj;
}
