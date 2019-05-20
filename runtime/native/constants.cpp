#include "constants.h"

extern "C" {
extern void Polyglot_jlang_runtime_Constants_load_class();
extern int Polyglot_jlang_runtime_Constants_classSize;
extern int Polyglot_jlang_runtime_Constants_numOfRuntimeCdvArrayMethods;
}

static int __classSize = 0;
static int __numOfRuntimeCdvArrayMethods = 0;

static void initConstants() {
    if (__classSize == 0 && __numOfRuntimeCdvArrayMethods == 0) {
        Polyglot_jlang_runtime_Constants_load_class();
        __classSize = Polyglot_jlang_runtime_Constants_classSize;
        __numOfRuntimeCdvArrayMethods =
            Polyglot_jlang_runtime_Constants_numOfRuntimeCdvArrayMethods;
    }
}

int getClassSize() {
    initConstants();
    return __classSize;
}

int getNumOfRuntimeArrayCdvMethods() {
    initConstants();
    return __numOfRuntimeCdvArrayMethods;
}