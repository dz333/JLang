// Copyright (C) 2018 Cornell University

#include "jvm.h"

#include "class.h"
#include "exception.h"
#include "factory.h"
#include "helper.h"
#include "jni.h"
#include "monitor.h"
#include "rep.h"
#include "signals.h"
#include "stack_trace.h"
#include "threads.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>
#include <pthread.h>

#define GC_THREADS
#include <gc.h>
#undef GC_THREADS

[[noreturn]] static void JvmUnimplemented(const char *name) {
    fprintf(stderr,
            "- - - - - - - - - - - - - - - - - - - - - - - - - - -\n"
            "The following JVM method is currently unimplemented:\n"
            "  %s\n"
            "It is defined in " __FILE__ ".\n"
            "Aborting for now.\n"
            "- - - - - - - - - - - - - - - - - - - - - - - - - - -\n",
            name);
    DumpStackTrace();
    abort();
}

#define MAX_PATH 2048
#define ARRAY_CLS "jlang.runtime.Array"

// vCode specified in openjdk/hotspot/src/share/vm/prims/jvm.h
#define JVM_T_BOOLEAN 4
#define JVM_T_CHAR 5
#define JVM_T_FLOAT 6
#define JVM_T_DOUBLE 7
#define JVM_T_BYTE 8
#define JVM_T_SHORT 9
#define JVM_T_INT 10
#define JVM_T_LONG 11

static void JvmIgnore(const char *name) {
    fprintf(stderr,
            "WARNING: JVM method %s is unimplemented, but will not abort.\n",
            name);
    fflush(stderr);
}

struct HashableShortArray {
    int len;
    unsigned short *chars;

    bool operator==(const HashableShortArray &other) const {
        if (len == other.len) {
            for (int i = 0; i < len; i++) {
                if (chars[i] != other.chars[i]) {
                    return false;
                }
            }
            return true;
        } else {
            return false;
        }
    }
};

namespace std {

template <> struct hash<HashableShortArray> {
    std::size_t operator()(const HashableShortArray k) const {
        std::size_t hash = 7;
        for (int i = 0; i < k.len; i++) {
            hash = hash * 31 + k.chars[i];
        }
        return hash;
    }
};

} // namespace std

std::unordered_map<HashableShortArray, jstring> InternedStrings;

jstring internJString(jstring str) {
    ScopedLock lock(Monitor::Instance().globalMutex());

    HashableShortArray *h =
        (HashableShortArray *)malloc(sizeof(HashableShortArray));
    h->len = (int)Unwrap(str)->Chars()->Length();
    h->chars = (unsigned short *)Unwrap(str)->Chars()->Data();
    auto search = InternedStrings.find(*h);
    if (search != InternedStrings.end()) {
        free(h);
        return search->second;
    } else {
        InternedStrings.insert({*h, str});
        return str;
    }
}

extern "C" {
// we copied this number from open JDK -> not sure the implications
#define JVM_INTERFACE_VERSION 4

jint JVM_GetInterfaceVersion(void) { return JVM_INTERFACE_VERSION; }

jint JVM_IHashCode(JNIEnv *env, jobject obj) {
    auto addr = reinterpret_cast<intptr_t>(obj);
    return static_cast<jint>(addr);
}

void JVM_MonitorWait(JNIEnv *env, jobject obj, jlong ms) {
    // check interrupted before waiting
    if (Threads::Instance().threads[currentThread].interrupted) {
        Threads::Instance().threads[currentThread].interrupted = false;
        throwInterruptedException(env);
    }
    Monitor::Instance().wait(obj, ms);
}

void JVM_MonitorNotify(JNIEnv *env, jobject obj) {
    Monitor::Instance().notify(obj);
}

void JVM_MonitorNotifyAll(JNIEnv *env, jobject obj) {
    Monitor::Instance().notifyAll(obj);
}

jobject JVM_Clone(JNIEnv *env, jobject obj) { return CloneJavaObject(obj); }

jstring JVM_InternString(JNIEnv *env, jstring str) {
    // TODO maybe implement this...maybe
    //   printf("string array elem size: %d\n",
    //   Unwrap(str)->Chars()->ElemSize());
    return internJString(str);
    //   return str;
}

jlong JVM_CurrentTimeMillis(JNIEnv *env, jclass ignored) {
    timeval t;
    if (gettimeofday(&t, NULL) == -1)
        JvmUnimplemented("CurrentTimeMillisFailed");
    return jlong(t.tv_sec) * 1000 + jlong(t.tv_usec) / 1000;
}

jlong JVM_NanoTime(JNIEnv *env, jclass ignored) {
    auto time = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(time).count();
}

void JVM_ArrayCopy(JNIEnv *env, jclass ignored, jobject src, jint src_pos,
                   jobject dst, jint dst_pos, jint length) {
    // TODO check bounds
    jarray src_arr = reinterpret_cast<jarray>(src);
    jarray dst_arr = reinterpret_cast<jarray>(dst);
    jsize elemsize = (Unwrap(src_arr)->ElemSize());
    char *src_data = static_cast<char *>(Unwrap(src_arr)->Data());
    char *dst_data = static_cast<char *>(Unwrap(dst_arr)->Data());
    memmove(dst_data + (dst_pos * elemsize), src_data + (src_pos * elemsize),
            elemsize * length);
}

#define PUTPROP(props, key, val)                                               \
    if (1) {                                                                   \
        jstring jkey = env->NewStringUTF(key);                                 \
        jstring jval = env->NewStringUTF(val);                                 \
        jobject r = env->CallObjectMethod(props, putID, jkey, jval);           \
        if (env->ExceptionOccurred())                                          \
            return NULL;                                                       \
        env->DeleteLocalRef(jkey);                                             \
        env->DeleteLocalRef(jval);                                             \
        env->DeleteLocalRef(r);                                                \
    } else                                                                     \
        ((void)0)

// '/jre/lib' for MAC
// '/jre/lib/amd64' for Linux
#ifdef __APPLE__
#define JRE_LIB "/jre/lib"
#define JRE_LIB_LEN 8
#else
#define JRE_LIB "/jre/lib/amd64"
#define JRE_LIB_LEN 14
#endif
jobject JVM_InitProperties(JNIEnv *env, jobject p) {
    // TODO add following system properties to p
    //  These are VM specific
    //  "java.vm.specification.name", "java.vm.version", "java.vm.name",
    //  "java.vm.info"

    // The following are OS specific, TODO is fill in the rest and support
    // multiple OS versions in a clener way
    // ""java.ext.dirs", "java.endorsed.dirs", "sun.boot.library.path",
    // "java.library.path", "java.home", "sun.boot.class.path"
    jmethodID putID = env->GetMethodID(
        env->GetObjectClass(p), "put",
        "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
    char *java_home = getenv(
        "JAVA_HOME"); // get java.home from JAVA_HOME environment variable
    if (java_home == NULL || java_home[0] == 0) {
        fprintf(stderr,
                "Could not find the JAVA_HOME variable in the environment.\n"
                "This must be set to the home directory of your JRE.\n");
        abort();
    }
    PUTPROP(p, "java.home", java_home);

    int dll_path_len = strlen(java_home) + JRE_LIB_LEN;
    char dll_path[dll_path_len];
    strcpy(dll_path, java_home);
    strcat(dll_path, JRE_LIB);
    PUTPROP(p, "sun.boot.library.path", dll_path);

    return p;
}

void JVM_OnExit(void (*func)(void)) { JvmUnimplemented("JVM_OnExit"); }

void JVM_Exit(jint code) {
    // TODO determine what else needs to happen on 'VM' shutdown
    // TODO call registered 'OnExit' functions
    exit(code);
}

void JVM_Halt(jint code) {
    // TODO determine what else needs to happen on 'VM' shutdown
    exit(code);
}

void JVM_GC(void) { JvmUnimplemented("JVM_GC"); }

jlong JVM_MaxObjectInspectionAge(void) {
    JvmUnimplemented("JVM_MaxObjectInspectionAge");
}

void JVM_TraceInstructions(jboolean on) {
    JvmUnimplemented("JVM_TraceInstructions");
}

void JVM_TraceMethodCalls(jboolean on) {
    JvmUnimplemented("JVM_TraceMethodCalls");
}

jlong JVM_TotalMemory(void) { JvmUnimplemented("JVM_TotalMemory"); }

jlong JVM_FreeMemory(void) {
    // dw475 TODO come back to this
    // always return 1mb for now
    return 0x100000;
}

jlong JVM_MaxMemory(void) { JvmUnimplemented("JVM_MaxMemory"); }

jint JVM_ActiveProcessorCount(void) { return sysconf(_SC_NPROCESSORS_ONLN); }

// TODO use our native lib $ from native.cpp
void *JVM_LoadLibrary(const char *name) {

    void *result = ::dlopen(name, RTLD_LAZY);
    if (result != NULL) {
        return result;
    } else {
        // TODO, report error info from system
        return NULL;
    }
}

void JVM_UnloadLibrary(void *handle) {
    ::dlclose(handle); // TODO other error handling required here?
}

void *JVM_FindLibraryEntry(void *handle, const char *name) {
    return dlsym(handle, name);
}

jboolean JVM_IsSupportedJNIVersion(jint version) {
    // TODO figure out real answer
    return JNI_TRUE;
}

jboolean JVM_IsNaN(jdouble d) { JvmUnimplemented("JVM_IsNaN"); }

void JVM_FillInStackTrace(JNIEnv *env, jobject throwable) {
    //    JvmIgnore("JVM_FillInStackTrace");
}

void JVM_PrintStackTrace(JNIEnv *env, jobject throwable, jobject printable) {
    JvmUnimplemented("JVM_PrintStackTrace");
}

jint JVM_GetStackTraceDepth(JNIEnv *env, jobject throwable) {
    JvmUnimplemented("JVM_GetStackTraceDepth");
}

jobject JVM_GetStackTraceElement(JNIEnv *env, jobject throwable, jint index) {
    JvmUnimplemented("JVM_GetStackTraceElement");
}

void JVM_InitializeCompiler(JNIEnv *env, jclass compCls) {
    JvmUnimplemented("JVM_InitializeCompiler");
}

jboolean JVM_IsSilentCompiler(JNIEnv *env, jclass compCls) {
    JvmUnimplemented("JVM_IsSilentCompiler");
}

jboolean JVM_CompileClass(JNIEnv *env, jclass compCls, jclass cls) {
    JvmUnimplemented("JVM_CompileClass");
}

jboolean JVM_CompileClasses(JNIEnv *env, jclass cls, jstring jname) {
    JvmUnimplemented("JVM_CompileClasses");
}

jobject JVM_CompilerCommand(JNIEnv *env, jclass compCls, jobject arg) {
    JvmUnimplemented("JVM_CompilerCommand");
}

void JVM_EnableCompiler(JNIEnv *env, jclass compCls) {
    JvmUnimplemented("JVM_EnableCompiler");
}

void JVM_DisableCompiler(JNIEnv *env, jclass compCls) { return; }

void JVM_StartThread(JNIEnv *env, jobject thread) {
    ScopedLock lock(Monitor::Instance().globalMutex());
    
    Threads::Instance().startThread(thread);
}

void JVM_StopThread(JNIEnv *env, jobject thread, jobject exception) {
    JvmUnimplemented("JVM_StopThread is Deprecated in Java8");
    return;
}

jboolean JVM_IsThreadAlive(JNIEnv *env, jobject thread) {
    return Threads::Instance().threads[thread].threadStatus;
}

void JVM_SuspendThread(JNIEnv *env, jobject thread) {
    JvmUnimplemented("JVM_SuspendThread is Deprecated in Java8");
    return;
}

void JVM_ResumeThread(JNIEnv *env, jobject thread) {
    JvmUnimplemented("JVM_ResumeThread is Deprecated in Java8");
    return;
}

void JVM_SetThreadPriority(JNIEnv *env, jobject thread, jint prio) {
    // TODO someday there will be synchronization
    return;
}

void JVM_Yield(JNIEnv *env, jclass threadClass) {
    sched_yield();
    return;
}

void JVM_Sleep(JNIEnv *env, jclass threadClass, jlong millis) {
    usleep(millis * 1000);
    return;
}

jobject JVM_CurrentThread(JNIEnv *env, jclass threadClass) {
    return currentThread;
}

jint JVM_CountStackFrames(JNIEnv *env, jobject thread) {
    JvmUnimplemented("JVM_CountStackFrames");
}

void JVM_Interrupt(JNIEnv *env, jobject thread) {
    // This is not a true scheduling primitive, so this function does not
    // acutally "interrupt" a thread. It only sets a boolean variable so that
    // the other thread could detect it.
    Threads::Instance().threads[thread].interrupted = true;
}

jboolean JVM_IsInterrupted(JNIEnv *env, jobject thread,
                           jboolean clearInterrupted) {
    bool interrupted = Threads::Instance().threads[thread].interrupted;
    Threads::Instance().threads[thread].interrupted &= !static_cast<bool>(clearInterrupted);
    return static_cast<jboolean>(interrupted);
}

jboolean JVM_HoldsLock(JNIEnv *env, jclass threadClass, jobject obj) {
    JvmUnimplemented("JVM_HoldsLock");
}

void JVM_DumpAllStacks(JNIEnv *env, jclass unused) {
    JvmUnimplemented("JVM_DumpAllStacks");
}

jobjectArray JVM_GetAllThreads(JNIEnv *env, jclass dummy) {
    JvmUnimplemented("JVM_GetAllThreads");
}

void JVM_SetNativeThreadName(JNIEnv *env, jobject jthread, jstring name) {
    JvmUnimplemented("JVM_SetNativeThreadName");
}

jobjectArray JVM_DumpThreads(JNIEnv *env, jclass threadClass,
                             jobjectArray threads) {
    JvmUnimplemented("JVM_DumpThreads");
}

jclass JVM_CurrentLoadedClass(JNIEnv *env) {
    JvmUnimplemented("JVM_CurrentLoadedClass");
}

jobject JVM_CurrentClassLoader(JNIEnv *env) {
    JvmUnimplemented("JVM_CurrentClassLoader");
}

jobjectArray JVM_GetClassContext(JNIEnv *env) {
    JvmUnimplemented("JVM_GetClassContext");
}

jint JVM_ClassDepth(JNIEnv *env, jstring name) {
    JvmUnimplemented("JVM_ClassDepth");
}

jint JVM_ClassLoaderDepth(JNIEnv *env) {
    JvmUnimplemented("JVM_ClassLoaderDepth");
}

jstring JVM_GetSystemPackage(JNIEnv *env, jstring name) {
    JvmUnimplemented("JVM_GetSystemPackage");
}

jobjectArray JVM_GetSystemPackages(JNIEnv *env) {
    JvmUnimplemented("JVM_GetSystemPackages");
}

jobject JVM_AllocateNewObject(JNIEnv *env, jobject obj, jclass currClass,
                              jclass initClass) {
    JvmUnimplemented("JVM_AllocateNewObject");
}

jobject JVM_AllocateNewArray(JNIEnv *env, jobject obj, jclass currClass,
                             jint length) {
    JvmUnimplemented("JVM_AllocateNewArray");
}

jobject JVM_LatestUserDefinedLoader(JNIEnv *env) {
    JvmUnimplemented("JVM_LatestUserDefinedLoader");
}

jclass JVM_LoadClass0(JNIEnv *env, jobject obj, jclass currClass,
                      jstring currClassName) {
    JvmUnimplemented("JVM_LoadClass0");
}

jint JVM_GetArrayLength(JNIEnv *env, jobject arr) {
    return reinterpret_cast<JArrayRep *>(arr)->Length();
}

jobject JVM_GetArrayElement(JNIEnv *env, jobject arr, jint index) {
    // dw475 TODO inspect this
    // return ((jobject*) reinterpret_cast<JArrayRep*>(arr)->Data())[index];
    return Polyglot_jlang_runtime_Helper_arrayLoad___3Ljava_lang_Object_2I(
        arr, index);
}

jvalue JVM_GetPrimitiveArrayElement(JNIEnv *env, jobject arr, jint index,
                                    jint wCode) {
    JvmUnimplemented("JVM_GetPrimitiveArrayElement");
}

void JVM_SetArrayElement(JNIEnv *env, jobject arr, jint index, jobject val) {
    // only non-primative objects
    assert(reinterpret_cast<JArrayRep *>(arr)->ElemSize() == sizeof(void *));
    ((void **)reinterpret_cast<JArrayRep *>(arr)->Data())[index] = val;
}

void JVM_SetPrimitiveArrayElement(JNIEnv *env, jobject arr, jint index,
                                  jvalue v, unsigned char vCode) {
    JvmUnimplemented("JVM_SetPrimitiveArrayElement");
}

jobject JVM_NewArray(JNIEnv *env, jclass eltClass, jint length) {
    const JavaClassInfo *info = GetJavaClassInfo(eltClass);
    if (isArrayClassName(info->name)) {
        // element is array.
        // [arr\0
        int len = strlen(info->name) + 1;
        char arrName[len + 1];
        arrName[0] = '[';
        strcpy(arrName + 1, info->name);
        return create1DArray(arrName, length);
    } else {
        char primName = primitiveNameToComponentName(info->name);
        if (primName != '\0') {
            // element is primitive.
            // [I\0
            char arrName[3] = {'[', primName, '\0'};
            return create1DArray(arrName, length);
        } else {
            // element is object.
            // [Lclassname;\0
            int len = strlen(info->name) + 3;
            char arrName[len + 1];
            arrName[0] = '[';
            arrName[1] = 'L';
            strcpy(arrName + 2, info->name);
            arrName[len - 1] = ';';
            arrName[len] = '\0';
            return create1DArray(arrName, length);
        }
    }
}

jobject JVM_NewMultiArray(JNIEnv *env, jclass eltClass, jintArray dim) {
    JvmUnimplemented("JVM_NewMultiArray");
}

jclass JVM_GetCallerClass(JNIEnv *env, int n) {
    //    JvmIgnore("JVM_GetCallerClass");
    return nullptr;
}

jclass JVM_FindPrimitiveClass(JNIEnv *env, const char *utf) {
    // TODO throw ClassNotFoundException if not primitive type
    return GetJavaClassFromName(utf);
}

void JVM_ResolveClass(JNIEnv *env, jclass cls) {
    JvmUnimplemented("JVM_ResolveClass");
}

jclass JVM_FindClassFromBootLoader(JNIEnv *env, const char *name) {
    JvmUnimplemented("JVM_FindClassFromBootLoader");
}

jclass JVM_FindClassFromCaller(JNIEnv *env, const char *name, jboolean init,
                               jobject loader, jclass caller) {
    ScopedLock lock(Monitor::Instance().globalMutex());

    jclass clazz = FindClassFromPathName(name);
    if (clazz == nullptr) {
        throwClassNotFoundException(env, name);
        return nullptr; // to make the compiler happy
    }
    return clazz;
}

jclass JVM_FindClassFromClassLoader(JNIEnv *env, const char *name,
                                    jboolean init, jobject loader,
                                    jboolean throwError) {
    JvmUnimplemented("JVM_FindClassFromClassLoader");
}

jclass JVM_FindClassFromClass(JNIEnv *env, const char *name, jboolean init,
                              jclass from) {
    JvmUnimplemented("JVM_FindClassFromClass");
}

jclass JVM_FindLoadedClass(JNIEnv *env, jobject loader, jstring name) {
    JvmUnimplemented("JVM_FindLoadedClass");
}

jclass JVM_DefineClass(JNIEnv *env, const char *name, jobject loader,
                       const jbyte *buf, jsize len, jobject pd) {
    JvmUnimplemented("JVM_DefineClass");
}

jclass JVM_DefineClassWithSource(JNIEnv *env, const char *name, jobject loader,
                                 const jbyte *buf, jsize len, jobject pd,
                                 const char *source) {
    JvmUnimplemented("JVM_DefineClassWithSource");
}

jstring JVM_GetClassName(JNIEnv *env, jclass cls) {
    auto name = GetJavaClassInfo(cls)->name;
    return env->NewStringUTF(name);
}

jobjectArray JVM_GetClassInterfaces(JNIEnv *env, jclass cls) {
    JvmUnimplemented("JVM_GetClassInterfaces");
}

jboolean JVM_IsInterface(JNIEnv *env, jclass cls) {
    auto info = GetJavaClassInfo(cls);
    if (info) {
        return info->isIntf;
    } else {
        return JNI_FALSE;
    }
}

jobjectArray JVM_GetClassSigners(JNIEnv *env, jclass cls) {
    JvmUnimplemented("JVM_GetClassSigners");
}

void JVM_SetClassSigners(JNIEnv *env, jclass cls, jobjectArray signers) {
    JvmUnimplemented("JVM_SetClassSigners");
}

jobject JVM_GetProtectionDomain(JNIEnv *env, jclass cls) {
    JvmUnimplemented("JVM_GetProtectionDomain");
}

void JVM_SetProtectionDomain(JNIEnv *env, jclass cls,
                             jobject protection_domain) {
    JvmUnimplemented("JVM_SetProtectionDomain");
}

jboolean JVM_IsArrayClass(JNIEnv *env, jclass cls) { return isArrayClass(cls); }

jboolean JVM_IsPrimitiveClass(JNIEnv *env, jclass cls) {
    return isPrimitiveClass(cls);
}

jclass JVM_GetComponentType(JNIEnv *env, jclass cls) {
    return GetComponentClass(cls);
}

jint JVM_GetClassModifiers(JNIEnv *env, jclass cls) {
    JvmUnimplemented("JVM_GetClassModifiers");
}

jobjectArray JVM_GetDeclaredClasses(JNIEnv *env, jclass ofClass) {
    JvmUnimplemented("JVM_GetDeclaredClasses");
}

jclass JVM_GetDeclaringClass(JNIEnv *env, jclass ofClass) {
    JvmUnimplemented("JVM_GetDeclaringClass");
}

jstring JVM_GetClassSignature(JNIEnv *env, jclass cls) {
    JvmUnimplemented("JVM_GetClassSignature");
}

jbyteArray JVM_GetClassAnnotations(JNIEnv *env, jclass cls) {
    JvmUnimplemented("JVM_GetClassAnnotations");
}

#define FIELD_INIT_FUNC                                                        \
    Polyglot_java_lang_reflect_Field_Field__Ljava_lang_Class_2Ljava_lang_String_2Ljava_lang_Class_2IILjava_lang_String_2_3B
#define METHOD_INIT_FUNC                                                       \
    Polyglot_java_lang_reflect_Method_Method__Ljava_lang_Class_2Ljava_lang_String_2_3Ljava_lang_Class_2Ljava_lang_Class_2_3Ljava_lang_Class_2IILjava_lang_String_2_3B_3B_3B

void FIELD_INIT_FUNC(jobject, jclass, jstring, jclass, jint, jint, jstring,
                     jbyteArray);
void METHOD_INIT_FUNC(jobject, jclass, jstring, jobjectArray, jclass,
                      jobjectArray, jint, jint, jstring, jbyteArray, jbyteArray,
                      jbyteArray);

std::vector<std::string> parseMethodSig(const std::string &sig) {
    std::vector<std::string> names;
    int start = 0;
    for (int i = 0; i < sig.size();) {
        int end;
        if (sig[i] == '(' || sig[i] == ')') {
            i++;
            start = i;
            continue;
        } else if (sig[i] == '[') { // array
            i++;
            continue;
        } else if (sig[i] == 'L') { // class
            end = sig.find(';', i);
        } else { // primitive
            end = i;
        }
        names.push_back(SigToClassName(sig.substr(start, end - start + 1)));
        i = end + 1;
        start = i;
    }
    return names;
}

jobjectArray JVM_GetClassDeclaredMethods(JNIEnv *env, jclass ofClass,
                                         jboolean publicOnly) {
    ScopedLock lock(Monitor::Instance().globalMutex());

    const JavaClassInfo *info = GetJavaClassInfo(ofClass);
    if (info) {
        const char *methodArrType = "[Ljava.lang.reflect.Method;";
        // if primative class (int, boolean), return empty array
        if (JVM_IsPrimitiveClass(env, ofClass)) {
            return (jobjectArray)create1DArray(methodArrType, 0);
        }

        jclass MethodClass = FindClass("java.lang.reflect.Method");

        // dw475 TODO take into account publiconly argument

        jobjectArray ret =
            (jobjectArray)create1DArray(methodArrType, info->num_methods);

        JavaMethodInfo *methods = info->methods;

        for (int i = 0; i < info->num_methods; i++) {
            // create new field object
            jobject newMethod = CreateJavaObject(MethodClass);
            jstring nameString = env->NewStringUTF(methods[i].name);
            jint modifiers = methods[i].modifiers;
            jint slot = i;
            std::vector<std::string> classNames =
                parseMethodSig(methods[i].sig);

            if (methods[i].returnType == nullptr) {
                methods[i].returnType =
                    new jclass(FindClass(classNames.back().c_str()));
            } else if (*methods[i].returnType == NULL) {
                FindClass(classNames.back().c_str());
            }
            jclass returnType = *methods[i].returnType;

            jobjectArray paramTypes = (jobjectArray)create1DArray(
                "[Ljava.lang.Class;", methods[i].numArgTypes);
            for (int k = 0; k < methods[i].numArgTypes; k++) {
                if (methods[i].argTypes[k] == nullptr) {
                    methods[i].argTypes[k] =
                        new jclass(FindClass(classNames[k].c_str()));
                } else if (*methods[i].argTypes[k] == nullptr) {
                    FindClass(classNames[k].c_str());
                }
                JVM_SetArrayElement(env, paramTypes, k,
                                    *methods[i].argTypes[k]);
            }

            jstring signature = env->NewStringUTF(methods[i].sig);

            // TODO: use the correct checkedExceptions
            jobjectArray checkedExceptions = (jobjectArray)create1DArray(
                "[Ljava.lang.Class;", 0);
            
            // TODO need to get the proper values
            // call the method constructor
            METHOD_INIT_FUNC(newMethod, ofClass, nameString, paramTypes,
                             returnType, checkedExceptions, modifiers, slot, NULL, NULL,
                             NULL, NULL);

            JVM_SetArrayElement(env, ret, i, newMethod);
        }

        return ret;
    }
    return NULL;
}

static std::unordered_map<jclass, jobjectArray> fieldsCache;

static jobjectArray _EmptyObjectArray = NULL;
jobjectArray getEmptyObjectArray() {
    if (_EmptyObjectArray == NULL) {
        _EmptyObjectArray = (jobjectArray)CreateJavaObjectArray(0);
    }
    return _EmptyObjectArray;
}

jobjectArray JVM_GetClassDeclaredFields(JNIEnv *env, jclass ofClass,
                                        jboolean publicOnly) {
    ScopedLock lock(Monitor::Instance().globalMutex());
    try {
        return fieldsCache.at(ofClass);
    } catch (const std::out_of_range &oor) {
    }

    const JavaClassInfo *info = GetJavaClassInfo(ofClass);
    if (info) {
        const char *fieldArrType = "[Ljava.lang.reflect.Field;";
        // if primative class (int, boolean), return empty array
        if (JVM_IsPrimitiveClass(env, ofClass)) {
            return (jobjectArray)create1DArray(fieldArrType, 0);
        }

        jclass FieldsClass = FindClass("java.lang.reflect.Field");

        // dw475 TODO take into account publiconly argument

        // non-static and static fields
        jobjectArray ret = (jobjectArray)create1DArray(
            fieldArrType, info->num_fields + info->num_static_fields);

        JavaFieldInfo *fields = info->fields;
        JavaStaticFieldInfo *staticFields = info->static_fields;

        for (int i = 0; i < info->num_fields + info->num_static_fields; i++) {
            // create new field object
            jobject newField = CreateJavaObject(FieldsClass);

            char *name = nullptr;
            int modifiers = 0;
            jclass **typePtrPtr = nullptr;
            char *signature = nullptr;
            int slot = 0;

            if (i < info->num_fields) {
                name = fields[i].name;
                modifiers = fields[i].modifiers;
                signature = fields[i].sig;
                typePtrPtr = &fields[i].type_ptr;
                slot = i;
            } else {
                int sidx = i - info->num_fields;
                name = staticFields[sidx].name;
                modifiers = staticFields[sidx].modifiers;
                signature = staticFields[sidx].sig;
                typePtrPtr = &staticFields[sidx].type_ptr;
                slot = -(sidx + 1); // 0 ambiguity
            }

            if (*typePtrPtr == nullptr) {
                // initialized Array's type_ptr if it is NULL.
                if (!isArrayClassName(signature)) {
                    printf("WARNING: Non-Array field has null type_ptr\n");
                }
                std::string className = SigToClassName(signature);
                *typePtrPtr = new jclass(FindClass(className.c_str()));
            } else if (**typePtrPtr == nullptr) {
                // The typeClass that type_ptr points to has not initialized
                // yet. Call the class loading function to load it.
                std::string className = SigToClassName(signature);
                FindClass(className.c_str());
                if (**typePtrPtr == nullptr) {
                    printf("WARNING: Class is not loaded correctly\n");
                }
            }

            jstring nameString = internJString(env->NewStringUTF(name));
            // Our field.sig is Java's string representation of field's type
            // Java's signature is used for Generics.
            // For non-generic type, the signature should be NULL.
            // For now, we don't support generics so we set sigString = NULL.
            // jstring sigString = env->NewStringUTF(signature);
            jstring sigString = NULL;

            // call the fields constructor
            FIELD_INIT_FUNC(newField, ofClass, nameString, **typePtrPtr,
                            modifiers, slot, sigString, NULL);
            // add it to the array
            JVM_SetArrayElement(env, ret, i, newField);
        }

        fieldsCache.emplace(ofClass, ret);

        return ret;
    }
    return NULL;
}

jobjectArray JVM_GetClassDeclaredConstructors(JNIEnv *env, jclass ofClass,
                                              jboolean publicOnly) {
    ScopedLock lock(Monitor::Instance().globalMutex());
    // TODO actually implement this - the following doesn't work yet
    auto class_info = GetJavaClassInfo(ofClass);
    if (class_info == NULL) {
        return (jobjectArray)create1DArray("[Ljava.lang.reflect.Constructor;",
                                           0);
    } else {
        return GetJavaConstructors(ofClass, class_info, publicOnly);
    }
}

jint JVM_GetClassAccessFlags(JNIEnv *env, jclass cls) {
    // dw475 TODO actually implement by adding getModifiers to class
    // public
    return 0x1;
    // JvmUnimplemented("JVM_GetClassAccessFlags");
}

jobject JVM_InvokeMethod(JNIEnv *env, jobject method, jobject obj,
                         jobjectArray args0) {
    ScopedLock lock(Monitor::Instance().globalMutex());

    const JavaClassInfo *info =
        GetJavaClassInfo(Unwrap(obj)->Cdv()->Class()->Wrap());

    int methodSlotOffset = 0;
    const JavaClassInfo *methodInfo =
        GetJavaClassInfo(Unwrap(method)->Cdv()->Class()->Wrap());
    for (int i = 0; i < methodInfo->num_fields; i++) {
        if (strcmp(methodInfo->fields[i].name, "slot") == 0) {
            methodSlotOffset = methodInfo->fields[i].offset;
            break;
        }
    }
    int slot = *((jint *)(((char *)method) + methodSlotOffset));

    jmethodID mtdId = reinterpret_cast<jmethodID>(&(info->methods[slot]));

    return CallJavaInstanceMethod<jobject>(
        obj, mtdId,
        (const jvalue *)reinterpret_cast<JArrayRep *>(args0)->Data());
}

jobject JVM_NewInstanceFromConstructor(JNIEnv *env, jobject c,
                                       jobjectArray args0) {
    JvmUnimplemented("JVM_NewInstanceFromConstructor");
}

jobject JVM_GetClassConstantPool(JNIEnv *env, jclass cls) {
    JvmUnimplemented("JVM_GetClassConstantPool");
}

jint JVM_ConstantPoolGetSize(JNIEnv *env, jobject unused, jobject jcpool) {
    JvmUnimplemented("JVM_ConstantPoolGetSize");
}

jclass JVM_ConstantPoolGetClassAt(JNIEnv *env, jobject unused, jobject jcpool,
                                  jint index) {
    JvmUnimplemented("JVM_ConstantPoolGetClassAt");
}

jclass JVM_ConstantPoolGetClassAtIfLoaded(JNIEnv *env, jobject unused,
                                          jobject jcpool, jint index) {
    JvmUnimplemented("JVM_ConstantPoolGetClassAtIfLoaded");
}

jobject JVM_ConstantPoolGetMethodAt(JNIEnv *env, jobject unused, jobject jcpool,
                                    jint index) {
    JvmUnimplemented("JVM_ConstantPoolGetMethodAt");
}

jobject JVM_ConstantPoolGetMethodAtIfLoaded(JNIEnv *env, jobject unused,
                                            jobject jcpool, jint index) {
    JvmUnimplemented("JVM_ConstantPoolGetMethodAtIfLoaded");
}

jobject JVM_ConstantPoolGetFieldAt(JNIEnv *env, jobject unused, jobject jcpool,
                                   jint index) {
    JvmUnimplemented("JVM_ConstantPoolGetFieldAt");
}

jobject JVM_ConstantPoolGetFieldAtIfLoaded(JNIEnv *env, jobject unused,
                                           jobject jcpool, jint index) {
    JvmUnimplemented("JVM_ConstantPoolGetFieldAtIfLoaded");
}

jobjectArray JVM_ConstantPoolGetMemberRefInfoAt(JNIEnv *env, jobject unused,
                                                jobject jcpool, jint index) {
    JvmUnimplemented("JVM_ConstantPoolGetMemberRefInfoAt");
}

jint JVM_ConstantPoolGetIntAt(JNIEnv *env, jobject unused, jobject jcpool,
                              jint index) {
    JvmUnimplemented("JVM_ConstantPoolGetIntAt");
}

jlong JVM_ConstantPoolGetLongAt(JNIEnv *env, jobject unused, jobject jcpool,
                                jint index) {
    JvmUnimplemented("JVM_ConstantPoolGetLongAt");
}

jfloat JVM_ConstantPoolGetFloatAt(JNIEnv *env, jobject unused, jobject jcpool,
                                  jint index) {
    JvmUnimplemented("JVM_ConstantPoolGetFloatAt");
}

jdouble JVM_ConstantPoolGetDoubleAt(JNIEnv *env, jobject unused, jobject jcpool,
                                    jint index) {
    JvmUnimplemented("JVM_ConstantPoolGetDoubleAt");
}

jstring JVM_ConstantPoolGetStringAt(JNIEnv *env, jobject unused, jobject jcpool,
                                    jint index) {
    JvmUnimplemented("JVM_ConstantPoolGetStringAt");
}

jstring JVM_ConstantPoolGetUTF8At(JNIEnv *env, jobject unused, jobject jcpool,
                                  jint index) {
    JvmUnimplemented("JVM_ConstantPoolGetUTF8At");
}

jobject JVM_DoPrivileged(JNIEnv *env, jclass cls, jobject action,
                         jobject context, jboolean wrapException) {
    // TODO throw NPE if action is null
    if (action == NULL) {
        fprintf(stderr, "ERROR: Null privileged action");
        return NULL;
    }
    // TODO we are just straight up calling the 'run' method, but doPrivileged
    // does a lot more
    auto pActionClazz = GetJavaClassFromName("java.security.PrivilegedAction");
    //  auto pActionClazz = jni_FindClass(env,
    //  "java/security/PrivilegedAction");
    jobject result = CallJavaInterfaceMethod<jobject>(
        action, pActionClazz, "run", "()Ljava/lang/Object;", NULL);
    return result;
}

jobject JVM_GetInheritedAccessControlContext(JNIEnv *env, jclass cls) {
    return NULL;
}

jobject JVM_GetStackAccessControlContext(JNIEnv *env, jclass cls) {
    // TODO is NULL ok here if we don't want to implement Access Control?
    // I don't know
    return NULL;
}

void *JVM_RegisterSignal(jint sig, void *handler) {
    // TODO really implement in signals.h
    return handler;
}

jboolean JVM_RaiseSignal(jint sig) {
    // TODO really implement in signals.h
    return JNI_TRUE;
}

jint JVM_FindSignal(const char *name) { return FindSignal(name); }

jboolean JVM_DesiredAssertionStatus(JNIEnv *env, jclass unused, jclass cls) {
    JvmUnimplemented("JVM_DesiredAssertionStatus");
}

jobject JVM_AssertionStatusDirectives(JNIEnv *env, jclass unused) {
    JvmUnimplemented("JVM_AssertionStatusDirectives");
}

jboolean JVM_SupportsCX8(void) { return JNI_FALSE; }

const char *JVM_GetClassNameUTF(JNIEnv *env, jclass cb) {
    auto name = GetJavaClassInfo(cb)->name;
    char *res = (char *)malloc(strlen(name) + 1);
    strcpy(res, name);
    return res;
}

void JVM_GetClassCPTypes(JNIEnv *env, jclass cb, unsigned char *types) {
    JvmUnimplemented("JVM_GetClassCPTypes");
}

jint JVM_GetClassCPEntriesCount(JNIEnv *env, jclass cb) {
    JvmUnimplemented("JVM_GetClassCPEntriesCount");
}

jint JVM_GetClassFieldsCount(JNIEnv *env, jclass cb) {
    JvmUnimplemented("JVM_GetClassFieldsCount");
}

jint JVM_GetClassMethodsCount(JNIEnv *env, jclass cb) {
    JvmUnimplemented("JVM_GetClassMethodsCount");
}

void JVM_GetMethodIxExceptionIndexes(JNIEnv *env, jclass cb, jint method_index,
                                     unsigned short *exceptions) {
    JvmUnimplemented("JVM_GetMethodIxExceptionIndexes");
}

jint JVM_GetMethodIxExceptionsCount(JNIEnv *env, jclass cb, jint method_index) {
    JvmUnimplemented("JVM_GetMethodIxExceptionsCount");
}

void JVM_GetMethodIxByteCode(JNIEnv *env, jclass cb, jint method_index,
                             unsigned char *code) {
    JvmUnimplemented("JVM_GetMethodIxByteCode");
}

jint JVM_GetMethodIxByteCodeLength(JNIEnv *env, jclass cb, jint method_index) {
    JvmUnimplemented("JVM_GetMethodIxByteCodeLength");
}

void JVM_GetMethodIxExceptionTableEntry(JNIEnv *env, jclass cb,
                                        jint method_index, jint entry_index,
                                        JVM_ExceptionTableEntryType *entry) {
    JvmUnimplemented("JVM_GetMethodIxExceptionTableEntry");
}

jint JVM_GetMethodIxExceptionTableLength(JNIEnv *env, jclass cb, int index) {
    JvmUnimplemented("JVM_GetMethodIxExceptionTableLength");
}

jint JVM_GetFieldIxModifiers(JNIEnv *env, jclass cb, int index) {
    JvmUnimplemented("JVM_GetFieldIxModifiers");
}

jint JVM_GetMethodIxModifiers(JNIEnv *env, jclass cb, int index) {
    JvmUnimplemented("JVM_GetMethodIxModifiers");
}

jint JVM_GetMethodIxLocalsCount(JNIEnv *env, jclass cb, int index) {
    JvmUnimplemented("JVM_GetMethodIxLocalsCount");
}

jint JVM_GetMethodIxArgsSize(JNIEnv *env, jclass cb, int index) {
    JvmUnimplemented("JVM_GetMethodIxArgsSize");
}

jint JVM_GetMethodIxMaxStack(JNIEnv *env, jclass cb, int index) {
    JvmUnimplemented("JVM_GetMethodIxMaxStack");
}

jboolean JVM_IsConstructorIx(JNIEnv *env, jclass cb, int index) {
    JvmUnimplemented("JVM_IsConstructorIx");
}

const char *JVM_GetMethodIxNameUTF(JNIEnv *env, jclass cb, jint index) {
    JvmUnimplemented("JVM_GetMethodIxNameUTF");
}

const char *JVM_GetMethodIxSignatureUTF(JNIEnv *env, jclass cb, jint index) {
    JvmUnimplemented("JVM_GetMethodIxSignatureUTF");
}

const char *JVM_GetCPFieldNameUTF(JNIEnv *env, jclass cb, jint index) {
    JvmUnimplemented("JVM_GetCPFieldNameUTF");
}

const char *JVM_GetCPMethodNameUTF(JNIEnv *env, jclass cb, jint index) {
    JvmUnimplemented("JVM_GetCPMethodNameUTF");
}

const char *JVM_GetCPMethodSignatureUTF(JNIEnv *env, jclass cb, jint index) {
    JvmUnimplemented("JVM_GetCPMethodSignatureUTF");
}

const char *JVM_GetCPFieldSignatureUTF(JNIEnv *env, jclass cb, jint index) {
    JvmUnimplemented("JVM_GetCPFieldSignatureUTF");
}

const char *JVM_GetCPClassNameUTF(JNIEnv *env, jclass cb, jint index) {
    JvmUnimplemented("JVM_GetCPClassNameUTF");
}

const char *JVM_GetCPFieldClassNameUTF(JNIEnv *env, jclass cb, jint index) {
    JvmUnimplemented("JVM_GetCPFieldClassNameUTF");
}

const char *JVM_GetCPMethodClassNameUTF(JNIEnv *env, jclass cb, jint index) {
    JvmUnimplemented("JVM_GetCPMethodClassNameUTF");
}

jint JVM_GetCPFieldModifiers(JNIEnv *env, jclass cb, int index,
                             jclass calledClass) {
    JvmUnimplemented("JVM_GetCPFieldModifiers");
}

jint JVM_GetCPMethodModifiers(JNIEnv *env, jclass cb, int index,
                              jclass calledClass) {
    JvmUnimplemented("JVM_GetCPMethodModifiers");
}

void JVM_ReleaseUTF(const char *utf) { free(const_cast<char *>(utf)); }

jboolean JVM_IsSameClassPackage(JNIEnv *env, jclass class1, jclass class2) {
    JvmUnimplemented("JVM_IsSameClassPackage");
}

jint JVM_GetLastErrorString(char *buf, int len) {
    JvmUnimplemented("JVM_GetLastErrorString");
}

char *JVM_NativePath(char *path) {
    // OS dependent
    return path;
}

// This is a JDK specific flag
// not a real UNIX flag, used by some native code.

#ifndef O_DELETE
#define O_DELETE 0x10000
#endif

jint JVM_Open(const char *path, jint oflag, jint mode) {

    if (strlen(path) > MAX_PATH - 1) {
        // JDK adds this, not sure how they user it
        // errno = ENAMETOOLONG;
        return -1;
    }
    int fd;
    int o_delete = (oflag & O_DELETE);
    oflag = oflag & ~O_DELETE;

    fd = open(path, oflag, mode);
    if (fd == -1)
        return -1;

    // If the open succeeded, the file might still be a directory
    {
        struct stat buf;
        int ret = fstat(fd, &buf);
        int st_mode = buf.st_mode;

        if (ret != -1) {
            if ((st_mode & S_IFMT) == S_IFDIR) {
                // JDK adds this, not sure how they user it
                // errno = EISDIR;
                close(fd);
                return -1;
            }
        } else {
            close(fd);
            return -1;
        }
    }
    // Comment in the JDK explains how some native code will break without
    // this
#ifdef FD_CLOEXEC
    {
        int flags = fcntl(fd, F_GETFD);
        if (flags != -1)
            fcntl(fd, F_SETFD, flags | FD_CLOEXEC);
    }
#endif

    if (o_delete != 0) {
        unlink(path);
    }
    return fd;
}

jint JVM_Close(jint fd) { return close(fd); }

jint JVM_Read(jint fd, char *buf, jint nbytes) {
    // TODO make repeatable on interrupt error
    return read(fd, buf, nbytes);
}

jint JVM_Write(jint fd, char *buf, jint nbytes) {
    jint result = -1;
    while (result == -1) {
        result = write(fd, buf, nbytes);
    }
    return result;
}

jint JVM_Available(jint fd, jlong *pbytes) {
    // mostly from JDK code, TODO understand and change
    jlong cur, end;
    int mode;
    struct stat buf;

    if (fstat(fd, &buf) >= 0) {
        mode = buf.st_mode;
        if (S_ISCHR(mode) || S_ISFIFO(mode) || S_ISSOCK(mode)) {
            /*
             * XXX: is the following call interruptible? If so, this might
             * need to go through the INTERRUPT_IO() wrapper as for other
             * blocking, interruptible calls in this file.
             */
            int n;
            if (ioctl(fd, FIONREAD, &n) >= 0) {
                *pbytes = n;
                return 1;
            }
        }
    }
    if ((cur = lseek(fd, 0L, SEEK_CUR)) == -1) {
        return 0;
    } else if ((end = lseek(fd, 0L, SEEK_END)) == -1) {
        return 0;
    } else if (lseek(fd, cur, SEEK_SET) == -1) {
        return 0;
    }
    *pbytes = end - cur;
    return 1;
}

jlong JVM_Lseek(jint fd, jlong offset, jint whence) {
    JvmUnimplemented("JVM_Lseek");
}

jint JVM_SetLength(jint fd, jlong length) { JvmUnimplemented("JVM_SetLength"); }

jint JVM_Sync(jint fd) { JvmUnimplemented("JVM_Sync"); }

jint JVM_InitializeSocketLibrary(void) {
    JvmUnimplemented("JVM_InitializeSocketLibrary");
}

jint JVM_Socket(jint domain, jint type, jint protocol) {
    JvmUnimplemented("JVM_Socket");
}

jint JVM_SocketClose(jint fd) { JvmUnimplemented("JVM_SocketClose"); }

jint JVM_SocketShutdown(jint fd, jint howto) {
    JvmUnimplemented("JVM_SocketShutdown");
}

jint JVM_Recv(jint fd, char *buf, jint nBytes, jint flags) {
    JvmUnimplemented("JVM_Recv");
}

jint JVM_Send(jint fd, char *buf, jint nBytes, jint flags) {
    JvmUnimplemented("JVM_Send");
}

jint JVM_Timeout(int fd, long timeout) { JvmUnimplemented("JVM_Timeout"); }

jint JVM_Listen(jint fd, jint count) { JvmUnimplemented("JVM_Listen"); }

jint JVM_Connect(jint fd, struct sockaddr *him, jint len) {
    JvmUnimplemented("JVM_Connect");
}

jint JVM_Bind(jint fd, struct sockaddr *him, jint len) {
    JvmUnimplemented("JVM_Bind");
}

jint JVM_Accept(jint fd, struct sockaddr *him, jint *len) {
    JvmUnimplemented("JVM_Accept");
}

jint JVM_RecvFrom(jint fd, char *buf, int nBytes, int flags,
                  struct sockaddr *from, int *fromlen) {
    JvmUnimplemented("JVM_RecvFrom");
}

jint JVM_SendTo(jint fd, char *buf, int len, int flags, struct sockaddr *to,
                int tolen) {
    JvmUnimplemented("JVM_SendTo");
}

jint JVM_SocketAvailable(jint fd, jint *result) {
    JvmUnimplemented("JVM_SocketAvailable");
}

jint JVM_GetSockName(jint fd, struct sockaddr *him, int *len) {
    JvmUnimplemented("JVM_GetSockName");
}

jint JVM_GetSockOpt(jint fd, int level, int optname, char *optval,
                    int *optlen) {
    JvmUnimplemented("JVM_GetSockOpt");
}

jint JVM_SetSockOpt(jint fd, int level, int optname, const char *optval,
                    int optlen) {
    JvmUnimplemented("JVM_SetSockOpt");
}

int JVM_GetHostName(char *name, int namelen) {
    JvmUnimplemented("JVM_GetHostName");
}

// FROM JDK (but what else would you write?
extern "C" {
int jio_vsnprintf(char *str, size_t count, const char *fmt, va_list args) {
    // see bug 4399518, 4417214
    if ((intptr_t)count <= 0)
        return -1;
    return vsnprintf(str, count, fmt, args);
}

int jio_snprintf(char *str, size_t count, const char *fmt, ...) {
    va_list args;
    int len;
    va_start(args, fmt);
    len = jio_vsnprintf(str, count, fmt, args);
    va_end(args);
    return len;
}

int jio_fprintf(FILE *f, const char *fmt, ...) {
    int len;
    va_list args;
    va_start(args, fmt);
    len = jio_vfprintf(f, fmt, args);
    va_end(args);
    return len;
}

int jio_vfprintf(FILE *f, const char *fmt, va_list args) {
    return vfprintf(f, fmt, args);
}
}

void *JVM_RawMonitorCreate(void) { JvmUnimplemented("JVM_RawMonitorCreate"); }

void JVM_RawMonitorDestroy(void *mon) {
    JvmUnimplemented("JVM_RawMonitorDestroy");
}

jint JVM_RawMonitorEnter(void *mon) { JvmUnimplemented("JVM_RawMonitorEnter"); }

void JVM_RawMonitorExit(void *mon) { JvmUnimplemented("JVM_RawMonitorExit"); }

void *JVM_GetManagement(jint version) { JvmUnimplemented("JVM_GetManagement"); }

jobject JVM_InitAgentProperties(JNIEnv *env, jobject agent_props) {
    JvmUnimplemented("JVM_InitAgentProperties");
}

jstring JVM_GetTemporaryDirectory(JNIEnv *env) {
    JvmUnimplemented("JVM_GetTemporaryDirectory");
}

jobjectArray JVM_GetEnclosingMethodInfo(JNIEnv *env, jclass ofClass) {
    JvmUnimplemented("JVM_GetEnclosingMethodInfo");
}

jintArray JVM_GetThreadStateValues(JNIEnv *env, jint javaThreadState) {
    JvmUnimplemented("JVM_GetThreadStateValues");
}

jobjectArray JVM_GetThreadStateNames(JNIEnv *env, jint javaThreadState,
                                     jintArray values) {
    JvmUnimplemented("JVM_GetThreadStateNames");
}

void JVM_GetVersionInfo(JNIEnv *env, jvm_version_info *info, size_t info_size) {
    //    JvmIgnore("JVM_GetVersionInfo");
}

void
JVM_GetClassTypeAnnotations(){
    JvmUnimplemented("JVM_GetClassTypeAnnotations");
}

} // extern "C"
