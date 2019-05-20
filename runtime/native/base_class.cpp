#include "base_class.h"

extern "C" {
extern void Polyglot_jlang_runtime_BaseClass_load_class();
extern jclass Polyglot_jlang_runtime_BaseClass_class;
}

jclass __baseClass = nullptr;

jclass getBaseClass() {
    if (__baseClass == NULL) {
        // Force this class load function to be called at initialization
        Polyglot_jlang_runtime_BaseClass_load_class();
        __baseClass = Polyglot_jlang_runtime_BaseClass_class;
    }
    return __baseClass;
}