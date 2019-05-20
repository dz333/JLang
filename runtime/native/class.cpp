// Copyright (C) 2018 Cornell University

// This file defines important data structures associated with
// Java class objects. These data structures are used to support
// JVM/JNI functionality, such as reflection.
#include "class.h"
#include "array.h"
#include "base_class.h"
#include "constants.h"
#include "jni.h"
#include "jvm.h"
#include "rep.h"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <gc.h>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <unordered_map>

#define MEMCPY(a, b, c) memcpy((void *)a, (void *)b, c)
static constexpr bool kDebug = false;
// static constexpr bool kDebug = true;

#define PRIM_CLASS(prim, klass) prim##klass
#define PRIM_CLASS_DEF(prim) static JClassRep *PRIM_CLASS(prim, Klass);

PRIM_CLASS_DEF(int)
PRIM_CLASS_DEF(short)
PRIM_CLASS_DEF(byte)
PRIM_CLASS_DEF(long)
PRIM_CLASS_DEF(float)
PRIM_CLASS_DEF(double)
PRIM_CLASS_DEF(char)
PRIM_CLASS_DEF(boolean)
PRIM_CLASS_DEF(void)

#define PRIM_NAME_CHECK(name, prim) if (strcmp(name, prim) == 0) {

#define PRIM_NAME_TO_CLASS(name, cname, prim)                                  \
    PRIM_NAME_CHECK(name, cname)                                               \
    return PRIM_CLASS(prim, Klass)->Wrap();

#define PRIM_IS_KLASS(cls, prim)                                               \
    if (Unwrap(cls) == PRIM_CLASS(prim, Klass)) {                              \
        return true;

#define PRIM_KLASS_SIZE(cls, prim, size)                                       \
    if (Unwrap(cls) == PRIM_CLASS(prim, Klass)) {                              \
        return size;

// dw475 TODO correctly set some fields
#define PRIM_REGISTER(prim)                                                    \
    if (1) {                                                                   \
        JavaClassInfo *newInfo =                                               \
            (JavaClassInfo *)malloc(sizeof(JavaClassInfo));                    \
        int nameLen = strlen(#prim) + 1;                                       \
        char *newName = (char *)malloc(nameLen);                               \
        memcpy(newName, #prim, nameLen);                                       \
        newInfo->name = newName;                                               \
        newInfo->isIntf = 0;                                                   \
        newInfo->num_intfs = 0;                                                \
        newInfo->num_fields = 0;                                               \
        newInfo->num_static_fields = 0;                                        \
        newInfo->num_methods = 0;                                              \
        newInfo->obj_size = classSize;                                         \
        newInfo->super_ptr = NULL;                                             \
        newInfo->cdv = NULL;                                                   \
        RegisterJavaClass(PRIM_CLASS(prim, Klass)->Wrap(), newInfo);           \
    } else                                                                     \
        ((void)0)

// just copy over DV
// dw475 TODO should copy over sync vars
#define REGISTER_PRIM_CLASS(prim)                                              \
    PRIM_CLASS(prim, Klass) = (JClassRep *)malloc(classSize);                  \
    memcpy(PRIM_CLASS(prim, Klass), baseClass, classSize);                     \
    Polyglot_native_##prim =                                                   \
        reinterpret_cast<jclass>(PRIM_CLASS(prim, Klass));                     \
    PRIM_REGISTER(prim);

static int classSize = 0;

// Define class references for JLang compiler
jclass Polyglot_native_int;
jclass Polyglot_native_byte;
jclass Polyglot_native_short;
jclass Polyglot_native_long;
jclass Polyglot_native_float;
jclass Polyglot_native_double;
jclass Polyglot_native_char;
jclass Polyglot_native_boolean;
jclass Polyglot_native_void;

// For simplicity we store class information in a map.
// If we find this to be too slow, we could allocate extra memory for
// class objects and store the information inline with each instance.
static std::unordered_map<jclass, const JavaClassInfo *> classes;
static std::unordered_map<std::string, const jclass> cnames;

extern "C" {

// invoked by JLang compiler to intern string literals when they are loaded
void InternStringLit(jstring str) { *str = *internJString(str); }

/**
 * Register a java class where cls points to the class object and
 * info points to the info object for that class
 */
void RegisterJavaClass(jclass cls, const JavaClassInfo *info) {
    if (kDebug) {
        printf("loading %s %s with super class %s\n",
               (info->isIntf ? "interface" : "class"), info->name,
               info->super_ptr ? GetJavaClassInfo(*info->super_ptr)->name
                               : "[none]");

        for (int32_t i = 0; i < info->num_intfs; ++i) {
            jclass *intf = info->intfs[i];
            printf("  implements interface %p\n", *intf);
        }
        for (int32_t i = 0; i < info->num_fields; ++i) {
            auto *f = &info->fields[i];
            printf("  found field %s with offset %d\n", f->name, f->offset);
        }

        for (int32_t i = 0; i < info->num_static_fields; ++i) {
            auto *f = &info->static_fields[i];
            printf("  found static field %s with sig %s and ptr %p\n", f->name,
                   f->sig, f->ptr);
        }

        for (int32_t i = 0; i < info->num_methods; ++i) {
            auto *m = &info->methods[i];
            printf("  found method %s%s\n"
                   "    offset %d\n"
                   "    function pointer %p\n"
                   "    trampoline pointer %p\n",
                   m->name, m->sig, m->offset, m->fnPtr, m->trampoline);
        }
    }
    // printf("loaded class %s\n", info->name);

    assert(classes.count(cls) == 0 && "Java class was loaded twice!");
    classes.emplace(cls, info);
    std::string cname(info->name);
    // if (strstr(info->name, "FieldReflection") != NULL) {
    //   printf("reg name: %s\n", info->name);
    // }
    cnames.emplace(cname, cls);
}

} // extern "C"

/**
 * Returns true if the class name in signature format is
 * an array class name.
 * This assumes char* is non-null, C-string with len > 0.
 * Examples:
 *  "I" returns false
 *  "[I" returns true
 *  "Ljava/lang/String;" returns false
 */
bool isArrayClassName(const char *name) { return name[0] == '['; }

/**
 * Returns true if cls is an array class
 */
bool isArrayClass(jclass cls) {
    auto cinfo = GetJavaClassInfo(cls);
    if (cinfo == NULL) {
        return JNI_FALSE;
    } else {
        return isArrayClassName(cinfo->name);
    }
}

/**
 * Returns the name of the component of the given name.
 * For example, if name is [[[I, this will return [[I.
 * This assumes char* is non-null, C-string with len >0
 */
const char *getComponentName(const char *name) { return &(name[1]); }

/**
 * Initialize an array class with the given name (should be
 * a type signature).
 * This assumes char* is non-null, C-string with len > 0.
 * It also assumes that it has not been initialized.
 * Returns the newly created array class
 */
const jclass initArrayClass(const char *name) {
    int jclass_size = classSize;
    if (jclass_size == 0) {
        printf("WARNING: class size not yet initialized\n");
        jclass_size = sizeof(JClassRep);
    }

    // create base array class and its info.
    jclass runtimeArrayClass = getRuntimeArrayClass();
    jclass newKlazz = (jclass)malloc(jclass_size);
    memcpy(newKlazz, runtimeArrayClass, jclass_size);
    JavaClassInfo *newInfo = (JavaClassInfo *)malloc(sizeof(JavaClassInfo));
    memcpy(newInfo, GetJavaClassInfo(runtimeArrayClass), sizeof(JavaClassInfo));

    // init and set name.
    int nameLen = strlen(name) + 1; // add the '\0' terminator
    char *newName = (char *)malloc(nameLen);
    memcpy(newName, name, nameLen);
    newInfo->name = newName;

    // init and set obj_size.
    newInfo->obj_size = sizeof(JArrayRep);

    // zero out fields and methods runtime information to conform
    // the reflectio behavior of Array. See jlang.runtime.Array
    // for a more detailed explanation.
    newInfo->num_fields = 0;
    newInfo->fields = nullptr;
    newInfo->num_static_fields = 0;
    newInfo->static_fields = nullptr;
    newInfo->num_methods = 0;
    newInfo->methods = nullptr;

    // init and set cdv/
    int numOfCdv = getNumOfRuntimeArrayCdvMethods();
    DispatchVector *runtimeArrayCdv = getRuntimeArrayCdv();
    int runtimeArrayCdvSize =
        sizeof(DispatchVector) + numOfCdv * sizeof(void *);
    DispatchVector *newCdv = (DispatchVector *)malloc(runtimeArrayCdvSize);
    memcpy(newCdv, runtimeArrayCdv, runtimeArrayCdvSize);
    newCdv->SetClassPtr(new JClassRep *(Unwrap(newKlazz)));
    newInfo->cdv = (void *)newCdv;

    RegisterJavaClass(newKlazz, newInfo);
    return newKlazz;
}

/**
 * Register the primative classes
 */
const void RegisterPrimitiveClasses() {

    jclass baseClass = getBaseClass();
    classSize = getClassSize();

    REGISTER_PRIM_CLASS(int)

    REGISTER_PRIM_CLASS(byte)

    REGISTER_PRIM_CLASS(short)

    REGISTER_PRIM_CLASS(long)

    REGISTER_PRIM_CLASS(float)

    REGISTER_PRIM_CLASS(double)

    REGISTER_PRIM_CLASS(char)

    REGISTER_PRIM_CLASS(boolean)

    REGISTER_PRIM_CLASS(void)
}

/**
 * Returns the primative class object for the given name
 * This assumes char* is non-null, C-string
 */
jclass primitiveComponentNameToClass(const char *name) {
    PRIM_NAME_TO_CLASS(name, "I", int)
    }
    else PRIM_NAME_TO_CLASS(name, "B", byte)
    }
    else PRIM_NAME_TO_CLASS(name, "S", short)
    }
    else PRIM_NAME_TO_CLASS(name, "J", long)
    }
    else PRIM_NAME_TO_CLASS(name, "F", float)
    }
    else PRIM_NAME_TO_CLASS(name, "D", double)
    }
    else PRIM_NAME_TO_CLASS(name, "C", char)
    }
    else PRIM_NAME_TO_CLASS(name, "Z", boolean)
    }
    else {
        return NULL;
    }
}

/**
 * Returns the component name for the given primitive type name.
 * If it is not a primitive type name, return '\0'.
 *
 * e.g. "int" -> 'I', "byte" -> 'B', "java.lang.Object" -> '\0'
 */
char primitiveNameToComponentName(const char *name) {
    PRIM_NAME_CHECK(name, "int") 
        return 'I';
    }
    else PRIM_NAME_CHECK(name, "byte") 
        return 'B';
    }
    else PRIM_NAME_CHECK(name, "short") 
        return 'S';
    }
    else PRIM_NAME_CHECK(name, "long") 
        return 'J';
    }
    else PRIM_NAME_CHECK(name, "float") 
        return 'F';
    }
    else PRIM_NAME_CHECK(name, "double") 
        return 'D';
    }
    else PRIM_NAME_CHECK(name, "char") 
        return 'C';
    }
    else PRIM_NAME_CHECK(name, "boolean") 
        return 'Z';
    }
    else {
        return '\0';
    }
}

/**
 * Returns true of the given class object points to a
 * primative class object
 */
bool isPrimitiveClass(jclass cls) { 
    PRIM_IS_KLASS(cls, int)
    }
    else PRIM_IS_KLASS(cls, byte)
    }
    else PRIM_IS_KLASS(cls, short)
    }
    else PRIM_IS_KLASS(cls, long)
    }
    else PRIM_IS_KLASS(cls, float)
    }
    else PRIM_IS_KLASS(cls, double)
    }
    else PRIM_IS_KLASS(cls, char)
    }
    else PRIM_IS_KLASS(cls, boolean)
    }
    else PRIM_IS_KLASS(cls, void)
    }
    else {
        return false;
    }
}

/**
 * Returns the number of bytes used to store the given type in an array
 * It respects size defined in LLVMUtils.sizeOfType(Type t).
 */
int arrayRepSize(jclass cls) { 
    PRIM_KLASS_SIZE(cls, int, 4) 
    }
    else PRIM_KLASS_SIZE(cls, byte, 1)
    }
    else PRIM_KLASS_SIZE(cls, short, 2)
    }
    else PRIM_KLASS_SIZE(cls, long, 8)
    }
    else PRIM_KLASS_SIZE(cls, float, 4)
    }
    else PRIM_KLASS_SIZE(cls, double, 8)
    }
    else PRIM_KLASS_SIZE(cls, char, 2)
    }
    else PRIM_KLASS_SIZE(cls, boolean, 1)
    }
    else PRIM_KLASS_SIZE(cls, void, 8)
    }
    else {
        return sizeof(void *);
    }
}
/**
 * Returns the class info object for the given java class object
 */
const JavaClassInfo *GetJavaClassInfo(jclass cls) {
    try {
        return classes.at(cls);
    } catch (const std::out_of_range &oor) {
        return NULL;
    }
}

/**
 * Returns the java class object for the given path
 * relative to the class path if the class has been loaded.
 * Example: java/lang/Class returns the Class class object.
 */
const jclass GetJavaClassFromPathName(const char *name) {
    int name_len = strlen(name);
    char path_name[name_len + 1];
    for (int i = 0; i <= name_len; i++) {
        char c = name[i];
        if (c == '/') {
            c = '.';
        }
        path_name[i] = c;
    }
    return GetJavaClassFromName(path_name);
}

/**
 * Returns the java class object for the given class name.
 * If the class has not been loaded yet and is not an array
 * class, this function will return null.
 * Example: java.lang.Class returns the Class class object
 */
const jclass GetJavaClassFromName(const char *name) {
    try {
        return cnames.at(std::string(name));
    } catch (const std::out_of_range &oor) {
        if (isArrayClassName(name)) {
            return initArrayClass(name);
        } else {
            return NULL;
        }
    }
}

DispatchVector *GetJavaCdvFromName(const char *name) {
    jclass clazz = GetJavaClassFromName(name);
    const JavaClassInfo *info = GetJavaClassInfo(clazz);
    return (DispatchVector *)info->cdv;
}

/**
 * Returns the class inside the given array class's array.
 * If the given class is not an array class, this function
 * returns null.
 */
jclass GetComponentClass(jclass cls) {
    if (isArrayClass(cls)) {
        const JavaClassInfo *info = GetJavaClassInfo(cls);
        if (info != NULL) {
            const char *componentName = getComponentName(info->name);
            jclass primComponent = primitiveComponentNameToClass(componentName);
            if (primComponent == NULL) {
                if (isArrayClassName(componentName)) {
                    return GetJavaClassFromName(componentName);
                } else {
                    int len = strlen(componentName) - 2;
                    char className[len + 1];
                    strncpy(className, componentName + 1, len);
                    className[len] = '\0';
                    return GetJavaClassFromName(className);
                }
            } else {
                return primComponent;
            }
        }
    }
    return NULL;
}

// Convert a jni type signature to its class name.
// len specifies the length of the signature string including the null
// terminator. className is required to point to a char array of length len.
// Rules: replace all slashes with dots.
// e.g. [Ljava/lang/String; -> [Ljava.lang.String;
std::string SigToClassName(const std::string &sig) {
    if (sig[0] == '[') { // array
        std::string name(sig);
        std::replace(name.begin(), name.end(), '/', '.');
        return name;
    } else if (sig[0] == 'L') { // class
        // get rid of head (L) and tail (;)
        std::string name = sig.substr(1, sig.size() - 2);
        std::replace(name.begin(), name.end(), '/', '.');
        return name;
    } else if (sig == "I") { // primitive type
        return "int";
    } else if (sig == "B") {
        return "byte";
    } else if (sig == "S") {
        return "short";
    } else if (sig == "J") {
        return "long";
    } else if (sig == "F") {
        return "float";
    } else if (sig == "D") {
        return "double";
    } else if (sig == "C") {
        return "char";
    } else if (sig == "Z") {
        return "boolean";
    } else if (sig == "V") {
        return "void";
    }
    throw std::invalid_argument("invalid signature to be converted: " + sig);
}

/**
 * Helper function to initialize an array in runtime.
 */
JArrayRep *createArrayHelper(const char *arrType, int *len, int depth) {
    const char *componentName = getComponentName(arrType);
    DispatchVector *cdv = GetJavaCdvFromName(arrType);

    jclass primComponent = primitiveComponentNameToClass(componentName);
    int elementSize;
    if (primComponent == NULL) {
        // any array or reference type
        elementSize = sizeof(void *);
    } else {
        elementSize = arrayRepSize(primComponent);
    }

    JArrayRep *arr =
        (JArrayRep *)GC_MALLOC(sizeof(JArrayRep) + elementSize * (*len));
    arr->Super()->SetCdv(cdv);
    arr->SetLength(*len);
    arr->SetElemSize(elementSize);
    // initialize elements when it is not leaf.
    // For leaf array, elements are 0 for all types.
    if (depth > 1) {
        void **data = (void **)arr->Data();
        for (int i = 0; i < (*len); ++i) {
            data[i] =
                (void *)createArrayHelper(componentName, len + 1, depth - 1);
        }
    }
    return arr;
}

jarray createArray(const char *arrType, int *len, int sizeOfLen) {
    JArrayRep *arr = createArrayHelper(arrType, len, sizeOfLen);
    return arr->Wrap();
}

jarray create1DArray(const char *arrType, int len) {
    const char *componentName = getComponentName(arrType);
    DispatchVector *cdv = GetJavaCdvFromName(arrType);

    jclass primComponent = primitiveComponentNameToClass(componentName);
    int elementSize;
    if (primComponent == NULL) {
        // any array or reference type
        elementSize = sizeof(void *);
    } else {
        elementSize = arrayRepSize(primComponent);
    }

    JArrayRep *arr =
        (JArrayRep *)GC_MALLOC(sizeof(JArrayRep) + elementSize * len);
    arr->Super()->SetCdv(cdv);
    arr->SetLength(len);
    arr->SetElemSize(elementSize);

    return arr->Wrap();
}

/**
 * Return the field information for the given class's static field
 */
const JavaStaticFieldInfo *GetJavaStaticFieldInfo(jclass cls, const char *name,
                                                  const char *sig) {
    auto *clazz = classes.at(cls);
    auto *fields = clazz->static_fields;
    for (int32_t i = 0, e = clazz->num_static_fields; i < e; ++i) {
        auto *f = &fields[i];
        if (strcmp(name, f->name) == 0 && strcmp(sig, f->sig) == 0) {
            return f;
        }
    }

    // TODO: Should technically throw NoSuchFieldError.
    fprintf(stderr, "Could not find static field %s in class %s. Aborting.\n",
            name, clazz->name);
    abort();
}

/**
 * Return the field information for the given class's field
 */
const JavaFieldInfo *GetJavaFieldInfo(jclass cls, const char *name) {
    auto *clazz = classes.at(cls);
    auto *fields = clazz->fields;
    for (int32_t i = 0, e = clazz->num_fields; i < e; ++i) {
        auto *f = &fields[i];
        if (strcmp(name, f->name) == 0) {
            return f;
        }
    }

    // TODO: Should technically throw NoSuchFieldError.
    fprintf(stderr, "Could not find field %s in class %s. Aborting.\n", name,
            clazz->name);
    abort();
}

const std::pair<JavaMethodInfo *, int32_t>
TryGetJavaMethodInfo(jclass cls, const char *name, const char *sig,
                     bool search_super) {
    auto *clazz = classes.at(cls);
    auto *methods = clazz->methods;
    for (int32_t i = 0, e = clazz->num_methods; i < e; ++i) {
        auto *m = &methods[i];
        if (strcmp(name, m->name) == 0 && strcmp(sig, m->sig) == 0) {
            return std::pair<JavaMethodInfo *, int32_t>(m, i);
        }
    }

    // Recurse to super class.
    if (search_super) {
        if (auto super_ptr = clazz->super_ptr) {
            // TODO: Technically might not want to recurse for 'private'
            // methods.
            return TryGetJavaMethodInfo(*super_ptr, name, sig, true);
        }
    }
    return std::pair<JavaMethodInfo *, int32_t>(nullptr, -1);
}

const std::pair<JavaMethodInfo *, int32_t>
GetJavaMethodInfo(jclass cls, const char *name, const char *sig) {
    return TryGetJavaMethodInfo(cls, name, sig, true);
}

const std::pair<JavaMethodInfo *, int32_t>
GetJavaStaticMethodInfo(jclass cls, const char *name, const char *sig) {
    return TryGetJavaMethodInfo(cls, name, sig, false);
}

// java.lang.Object -> Polyglot_java_lang_Object_load_class
#define LOADER_PREFIX "Polyglot_"
#define LOADER_PREFIX_LEN 9
#define LOADER_SUFFIX "_load_class"
#define LOADER_SUFFIX_LEN 10
#define LOADER_NAME_CHARS (LOADER_PREFIX_LEN + LOADER_SUFFIX_LEN)

typedef jclass (*class_loader)();

/**
 * Load a class with the given name into the executable.
 * The class name can be in the format java/lang/Class or
 * java.lang.Class
 */
jclass LoadJavaClassFromLib(const char *name) {
    int name_len = strlen(name);
    int new_len = name_len + LOADER_NAME_CHARS + 1;
    char class_load_name[new_len];
    strcpy(class_load_name, LOADER_PREFIX);
    int i = 0;
    for (; i < name_len; i++) {
        char next_char = name[i];
        class_load_name[LOADER_PREFIX_LEN + i] =
            (next_char == '.' || next_char == '/') ? '_' : next_char;
    }
    class_load_name[LOADER_PREFIX_LEN + i] = '\0';
    strcat(class_load_name, LOADER_SUFFIX);
    auto class_load_func =
        reinterpret_cast<class_loader>(dlsym(RTLD_DEFAULT, class_load_name));
    if (class_load_func != NULL) {
        return class_load_func();
    } else {
        return NULL;
    }
}
