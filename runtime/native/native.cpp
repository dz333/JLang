// Copyright (C) 2018 Cornell University

#include "class.h"
#include "jni.h"
#include "stack_trace.h"
#include <cstdlib>
#include <dlfcn.h>
#include <string>
#include <tuple>
#include <unordered_map>

#include "native.h"

static constexpr bool kDebug = false;

// This serves as a cache for native method lookups,
// as well as a way to register methods through JNI.
static std::unordered_map<std::string, void *> native_map;

// Builds a unique identifier for native methods.
// E.g., java.lang.Object#wait(J)V.
//
// This intentionally builds on the format used in the
// JNI method registerNatives();
static std::string BuildJavaNativeFuncKey(jclass cls, // e.g., java.lang.Object
                                          const char *name,     // e.g., wait
                                          const char *signature // e.g., (J)V
) {
    std::string key;
    key += GetJavaClassInfo(cls)->name;
    key += '#';
    key += name;
    key += signature;
    return key;
}

void RegisterJavaNativeFunc(jclass cls,            // e.g., java.lang.Object
                            const char *name,      // e.g., wait
                            const char *signature, // e.g., (J)V
                            void *func) {
    if (kDebug)
        printf("[runtime] registering native method %s%s\n", name, signature);

    auto key = BuildJavaNativeFuncKey(cls, name, signature);

    decltype(native_map)::iterator it;
    bool success;
    tie(it, success) = native_map.emplace(key, func);
    if (!success) {
        // The native method was already linked; replace it.
        it->second = func;
    }
}

extern "C" void *
GetJavaNativeFunc(jclass cls,               // e.g., java.lang.Object
                  const char *name,         // e.g., wait
                  const char *signature,    // e.g., (J)V
                  const char *short_symbol, // e.g., Java_java_lang_Object_wait
                  const char *long_symbol // e.g., Java_java_lang_Object_wait__J
) {
    // Check cache.
    auto key = BuildJavaNativeFuncKey(cls, name, signature);
    auto it = native_map.find(key);
    if (it != native_map.end()) {
        if (kDebug)
            printf("[runtime] found cached native method %s\n", key.c_str());
        return it->second;
    }

    // Search for symbol by short name first, then long name.
    for (const char *symbol : {short_symbol, long_symbol}) {
        if (void *func = dlsym(RTLD_DEFAULT, symbol)) {
            if (kDebug)
                printf("[runtime] found native method symbol %s\n", symbol);
            native_map.emplace(key, func);
            return func;
        }
    }

    fprintf(stderr,
            "- - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n"
            "Link error.\n"
            "The following Java native method is unlinked:\n"
            "  %s\n"
            "Aborting for now.\n"
            "- - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n",
            short_symbol);
    DumpStackTrace();
    abort();
}
