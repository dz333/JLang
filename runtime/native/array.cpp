#include "array.h"

extern "C" {
extern void Polyglot_jlang_runtime_Array_load_class();
extern DispatchVector Polyglot_jlang_runtime_Array_cdv;
extern jclass Polyglot_jlang_runtime_Array_class;
}

static jclass __runtimeArrayClass = nullptr;
static DispatchVector *__runtimeArrayCdv = nullptr;

static void initRuntimeArray() {
    if (__runtimeArrayClass == nullptr && __runtimeArrayCdv == nullptr) {
        Polyglot_jlang_runtime_Array_load_class();
        __runtimeArrayClass = Polyglot_jlang_runtime_Array_class;
        __runtimeArrayCdv = &Polyglot_jlang_runtime_Array_cdv;
    }
}

jclass getRuntimeArrayClass() {
    initRuntimeArray();
    return __runtimeArrayClass;
}

DispatchVector *getRuntimeArrayCdv() {
    initRuntimeArray();
    return __runtimeArrayCdv;
}