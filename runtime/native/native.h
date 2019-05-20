// Copyright (C) 2018 Cornell University

// This file supports dynamically looking up Java native method pointers.
// We cannot link to native methods ahead-of-time, because native methods
// are often registered with the VM dynamically at runtime.
#pragma once

#include "jni.h"

// Links a native method to the given function pointer.
void RegisterJavaNativeFunc(jclass cls,            // e.g., java.lang.Object
                            const char *name,      // e.g., wait
                            const char *signature, // e.g., (J)V
                            void *func);

// Returns a pointer to a Java native method.
//
// The name and signature are used in the case that the
// native method has been registered dynamically through JNI,
// or in the case that the native method has been cached.
//
// The short symbol and long symbol are used in case the native method
// needs to be searched for in the symbol table.
//
// The signature of this function must precisely match
// the signature used in JLang.
extern "C" void *
GetJavaNativeFunc(jclass cls,               // e.g., java.lang.Object
                  const char *name,         // e.g., wait
                  const char *signature,    // e.g., (J)V
                  const char *short_symbol, // e.g., Java_java_lang_Object_wait
                  const char *long_symbol // e.g., Java_java_lang_Object_wait__J
);
