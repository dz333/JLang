// Copyright (C) 2018 Cornell University

#include "nativetest.h"

extern "C" {
JNIEXPORT jstring JNICALL Java_NativeTest_getNativeStringTest(JNIEnv *env,
                                                              jclass ignored) {
    jstring jkey = env->NewStringUTF("testNativeCall");
    return jkey;
}

JNIEXPORT jstring JNICALL
Java_NativeTest_00024NativeTestInner_getNativeInnerStringTest(JNIEnv *env,
                                                              jobject ignored) {
    jstring jkey = env->NewStringUTF("testInnerNativeCall");
    return jkey;
}
}
