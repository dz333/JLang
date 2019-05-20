// Copyright (C) 2018 Cornell University

#include "reflect.h"

extern "C" {

void CTOR_CTOR(jclass, jobjectArray, jobjectArray, jint, jint, jstring,
               jbyteArray, jbyteArray);

bool InstanceOf(jobject obj, void *type_id) {
    if (obj == nullptr)
        return false;
    type_info *type_info = Unwrap(obj)->Cdv()->SuperTypes();
    for (int32_t i = 0, end = type_info->size; i < end; ++i)
        if (type_info->super_type_ids[i] == type_id)
            return true;
    return false;
}
} // extern "C"

jobject CreateConstructor(jclass declaring_clazz,
                          const JavaClassInfo *clazz_info,
                          JavaMethodInfo ctor_info) {
    // TODO actually implement
    return NULL;
}
