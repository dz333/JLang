// Copyright (C) 2018 Cornell University

// Provides definitions to java runtime functions compiled by JLang

#pragma once

#include "jni.h"

#define POLYGLOT_ARRAY_STORE                                                   \
    Polyglot_jlang_runtime_Helper_arrayStore___3Ljava_lang_Object_2ILjava_lang_Object_2

extern "C" {
extern jstring
    Polyglot_jlang_runtime_Helper_toString__Ljava_lang_Object_2(jobject);
void POLYGLOT_ARRAY_STORE(jobjectArray, jint, jobject);
extern jobject
    Polyglot_jlang_runtime_Helper_arrayLoad___3Ljava_lang_Object_2I(jobject,
                                                                    jint);
extern void
    Polyglot_jlang_runtime_Helper_printString__Ljava_lang_String_2(jstring);
}