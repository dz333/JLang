//Copyright (C) 2018 Cornell University

#pragma once

#include "jni.h"
#include "jni_help.h"

extern "C" {

jstring internJString(jstring str);

jint
JVM_GetInterfaceVersion(void);

jint
JVM_IHashCode(JNIEnv *env, jobject obj);

void
JVM_MonitorWait(JNIEnv *env, jobject obj, jlong ms);

void
JVM_MonitorNotify(JNIEnv *env, jobject obj);

void
JVM_MonitorNotifyAll(JNIEnv *env, jobject obj);

jobject
JVM_Clone(JNIEnv *env, jobject obj);

jstring
JVM_InternString(JNIEnv *env, jstring str);

jlong
JVM_CurrentTimeMillis(JNIEnv *env, jclass ignored);

jlong
JVM_NanoTime(JNIEnv *env, jclass ignored);

void
JVM_ArrayCopy(JNIEnv *env, jclass ignored, jobject src, jint src_pos, jobject dst, jint dst_pos, jint length);

jobject
JVM_InitProperties(JNIEnv *env, jobject p);

void
JVM_OnExit(void (*func)(void));

void
JVM_Exit(jint code);

void
JVM_Halt(jint code);

void
JVM_GC(void);

jlong
JVM_MaxObjectInspectionAge(void);

void
JVM_TraceInstructions(jboolean on);

void
JVM_TraceMethodCalls(jboolean on);

jlong
JVM_TotalMemory(void);

jlong
JVM_FreeMemory(void);

jlong
JVM_MaxMemory(void);

jint
JVM_ActiveProcessorCount(void);

void*
JVM_LoadLibrary(const char *name);

void
JVM_UnloadLibrary(void * handle);

void*
JVM_FindLibraryEntry(void *handle, const char *name);

jboolean
JVM_IsSupportedJNIVersion(jint version);

jboolean
JVM_IsNaN(jdouble d);

void
JVM_FillInStackTrace(JNIEnv *env, jobject throwable);

void
JVM_PrintStackTrace(JNIEnv *env, jobject throwable, jobject printable);

jint
JVM_GetStackTraceDepth(JNIEnv *env, jobject throwable);

jobject
JVM_GetStackTraceElement(JNIEnv *env, jobject throwable, jint index);

void
JVM_InitializeCompiler(JNIEnv *env, jclass compCls);

jboolean
JVM_IsSilentCompiler(JNIEnv *env, jclass compCls);

jboolean
JVM_CompileClass(JNIEnv *env, jclass compCls, jclass cls);

jboolean
JVM_CompileClasses(JNIEnv *env, jclass cls, jstring jname);

jobject
JVM_CompilerCommand(JNIEnv *env, jclass compCls, jobject arg);

void
JVM_EnableCompiler(JNIEnv *env, jclass compCls);

void
JVM_DisableCompiler(JNIEnv *env, jclass compCls);

void
JVM_StartThread(JNIEnv *env, jobject thread);

void
JVM_StopThread(JNIEnv *env, jobject thread, jobject exception);

jboolean
JVM_IsThreadAlive(JNIEnv *env, jobject thread);

void
JVM_SuspendThread(JNIEnv *env, jobject thread);

void
JVM_ResumeThread(JNIEnv *env, jobject thread);

void
JVM_SetThreadPriority(JNIEnv *env, jobject thread, jint prio);

void
JVM_Yield(JNIEnv *env, jclass threadClass);

void
JVM_Sleep(JNIEnv *env, jclass threadClass, jlong millis);

jobject
JVM_CurrentThread(JNIEnv *env, jclass threadClass);

jint
JVM_CountStackFrames(JNIEnv *env, jobject thread);

void
JVM_Interrupt(JNIEnv *env, jobject thread);

jboolean
JVM_IsInterrupted(JNIEnv *env, jobject thread, jboolean clearInterrupted);

jboolean
JVM_HoldsLock(JNIEnv *env, jclass threadClass, jobject obj);

void
JVM_DumpAllStacks(JNIEnv *env, jclass unused);

jobjectArray
JVM_GetAllThreads(JNIEnv *env, jclass dummy);

void
JVM_SetNativeThreadName(JNIEnv *env, jobject jthread, jstring name);

jobjectArray
JVM_DumpThreads(JNIEnv *env, jclass threadClass, jobjectArray threads);

jclass
JVM_CurrentLoadedClass(JNIEnv *env);

jobject
JVM_CurrentClassLoader(JNIEnv *env);

jobjectArray
JVM_GetClassContext(JNIEnv *env);

jint
JVM_ClassDepth(JNIEnv *env, jstring name);

jint
JVM_ClassLoaderDepth(JNIEnv *env);

jstring
JVM_GetSystemPackage(JNIEnv *env, jstring name);

jobjectArray
JVM_GetSystemPackages(JNIEnv *env);

jobject
JVM_AllocateNewObject(JNIEnv *env, jobject obj, jclass currClass, jclass initClass);

jobject
JVM_AllocateNewArray(JNIEnv *env, jobject obj, jclass currClass, jint length);

jobject
JVM_LatestUserDefinedLoader(JNIEnv *env);

jclass
JVM_LoadClass0(JNIEnv *env, jobject obj, jclass currClass, jstring currClassName);

jint
JVM_GetArrayLength(JNIEnv *env, jobject arr);

jobject
JVM_GetArrayElement(JNIEnv *env, jobject arr, jint index);

jvalue
JVM_GetPrimitiveArrayElement(JNIEnv *env, jobject arr, jint index, jint wCode);

void
JVM_SetArrayElement(JNIEnv *env, jobject arr, jint index, jobject val);

void
JVM_SetPrimitiveArrayElement(JNIEnv *env, jobject arr, jint index, jvalue v, unsigned char vCode);

jobject
JVM_NewArray(JNIEnv *env, jclass eltClass, jint length);

jobject
JVM_NewMultiArray(JNIEnv *env, jclass eltClass, jintArray dim);

jclass
JVM_GetCallerClass(JNIEnv *env, int n);

jclass
JVM_FindPrimitiveClass(JNIEnv *env, const char *utf);

void
JVM_ResolveClass(JNIEnv *env, jclass cls);

jclass
JVM_FindClassFromBootLoader(JNIEnv *env, const char *name);

jclass
JVM_FindClassFromCaller(JNIEnv *env, const char *name, jboolean init, jobject loader, jclass caller);

jclass
JVM_FindClassFromClassLoader(JNIEnv *env, const char *name, jboolean init, jobject loader, jboolean throwError);

jclass
JVM_FindClassFromClass(JNIEnv *env, const char *name, jboolean init, jclass from);

jclass
JVM_FindLoadedClass(JNIEnv *env, jobject loader, jstring name);

jclass
JVM_DefineClass(JNIEnv *env, const char *name, jobject loader, const jbyte *buf, jsize len, jobject pd);

jclass
JVM_DefineClassWithSource(JNIEnv *env, const char *name, jobject loader, const jbyte *buf, jsize len, jobject pd, const char *source);

jstring
JVM_GetClassName(JNIEnv *env, jclass cls);

jobjectArray
JVM_GetClassInterfaces(JNIEnv *env, jclass cls);

jboolean
JVM_IsInterface(JNIEnv *env, jclass cls);

jobjectArray
JVM_GetClassSigners(JNIEnv *env, jclass cls);

void
JVM_SetClassSigners(JNIEnv *env, jclass cls, jobjectArray signers);

jobject
JVM_GetProtectionDomain(JNIEnv *env, jclass cls);

void
JVM_SetProtectionDomain(JNIEnv *env, jclass cls, jobject protection_domain);

jboolean
JVM_IsArrayClass(JNIEnv *env, jclass cls);

jboolean
JVM_IsPrimitiveClass(JNIEnv *env, jclass cls);

jclass
JVM_GetComponentType(JNIEnv *env, jclass cls);

jint
JVM_GetClassModifiers(JNIEnv *env, jclass cls);

jobjectArray
JVM_GetDeclaredClasses(JNIEnv *env, jclass ofClass);

jclass
JVM_GetDeclaringClass(JNIEnv *env, jclass ofClass);

jstring
JVM_GetClassSignature(JNIEnv *env, jclass cls);

jbyteArray
JVM_GetClassAnnotations(JNIEnv *env, jclass cls);

jobjectArray
JVM_GetClassDeclaredMethods(JNIEnv *env, jclass ofClass, jboolean publicOnly);

jobjectArray
JVM_GetClassDeclaredFields(JNIEnv *env, jclass ofClass, jboolean publicOnly);

jobjectArray
JVM_GetClassDeclaredConstructors(JNIEnv *env, jclass ofClass, jboolean publicOnly);

jint
JVM_GetClassAccessFlags(JNIEnv *env, jclass cls);

jobject
JVM_InvokeMethod(JNIEnv *env, jobject method, jobject obj, jobjectArray args0);

jobject
JVM_NewInstanceFromConstructor(JNIEnv *env, jobject c, jobjectArray args0);

jobject
JVM_GetClassConstantPool(JNIEnv *env, jclass cls);

jint
JVM_ConstantPoolGetSize(JNIEnv *env, jobject unused, jobject jcpool);

jclass
JVM_ConstantPoolGetClassAt(JNIEnv *env, jobject unused, jobject jcpool, jint index);

jclass
JVM_ConstantPoolGetClassAtIfLoaded(JNIEnv *env, jobject unused, jobject jcpool, jint index);

jobject
JVM_ConstantPoolGetMethodAt(JNIEnv *env, jobject unused, jobject jcpool, jint index);

jobject
JVM_ConstantPoolGetMethodAtIfLoaded(JNIEnv *env, jobject unused, jobject jcpool, jint index);

jobject
JVM_ConstantPoolGetFieldAt(JNIEnv *env, jobject unused, jobject jcpool, jint index);

jobject
JVM_ConstantPoolGetFieldAtIfLoaded(JNIEnv *env, jobject unused, jobject jcpool, jint index);

jobjectArray
JVM_ConstantPoolGetMemberRefInfoAt(JNIEnv *env, jobject unused, jobject jcpool, jint index);

jint
JVM_ConstantPoolGetIntAt(JNIEnv *env, jobject unused, jobject jcpool, jint index);

jlong
JVM_ConstantPoolGetLongAt(JNIEnv *env, jobject unused, jobject jcpool, jint index);

jfloat
JVM_ConstantPoolGetFloatAt(JNIEnv *env, jobject unused, jobject jcpool, jint index);

jdouble
JVM_ConstantPoolGetDoubleAt(JNIEnv *env, jobject unused, jobject jcpool, jint index);

jstring
JVM_ConstantPoolGetStringAt(JNIEnv *env, jobject unused, jobject jcpool, jint index);

jstring
JVM_ConstantPoolGetUTF8At(JNIEnv *env, jobject unused, jobject jcpool, jint index);

jobject
JVM_DoPrivileged(JNIEnv *env, jclass cls, jobject action, jobject context, jboolean wrapException);

jobject
JVM_GetInheritedAccessControlContext(JNIEnv *env, jclass cls);

jobject
JVM_GetStackAccessControlContext(JNIEnv *env, jclass cls);

void*
JVM_RegisterSignal(jint sig, void *handler);

jboolean
JVM_RaiseSignal(jint sig);

jint
JVM_FindSignal(const char *name);

jboolean
JVM_DesiredAssertionStatus(JNIEnv *env, jclass unused, jclass cls);

jobject
JVM_AssertionStatusDirectives(JNIEnv *env, jclass unused);

jboolean
JVM_SupportsCX8(void);

const char*
JVM_GetClassNameUTF(JNIEnv *env, jclass cb);

void
JVM_GetClassCPTypes(JNIEnv *env, jclass cb, unsigned char *types);

jint
JVM_GetClassCPEntriesCount(JNIEnv *env, jclass cb);

jint
JVM_GetClassFieldsCount(JNIEnv *env, jclass cb);

jint
JVM_GetClassMethodsCount(JNIEnv *env, jclass cb);

void
JVM_GetMethodIxExceptionIndexes(JNIEnv *env, jclass cb, jint method_index, unsigned short *exceptions);

jint
JVM_GetMethodIxExceptionsCount(JNIEnv *env, jclass cb, jint method_index);

void
JVM_GetMethodIxByteCode(JNIEnv *env, jclass cb, jint method_index, unsigned char *code);

jint
JVM_GetMethodIxByteCodeLength(JNIEnv *env, jclass cb, jint method_index);

typedef struct {
    jint start_pc;
    jint end_pc;
    jint handler_pc;
    jint catchType;
} JVM_ExceptionTableEntryType;

void
JVM_GetMethodIxExceptionTableEntry(JNIEnv *env, jclass cb, jint method_index, jint entry_index, JVM_ExceptionTableEntryType *entry);

jint
JVM_GetMethodIxExceptionTableLength(JNIEnv *env, jclass cb, int index);

jint
JVM_GetFieldIxModifiers(JNIEnv *env, jclass cb, int index);

jint
JVM_GetMethodIxModifiers(JNIEnv *env, jclass cb, int index);

jint
JVM_GetMethodIxLocalsCount(JNIEnv *env, jclass cb, int index);

jint
JVM_GetMethodIxArgsSize(JNIEnv *env, jclass cb, int index);

jint
JVM_GetMethodIxMaxStack(JNIEnv *env, jclass cb, int index);

jboolean
JVM_IsConstructorIx(JNIEnv *env, jclass cb, int index);

const char*
JVM_GetMethodIxNameUTF(JNIEnv *env, jclass cb, jint index);

const char*
JVM_GetMethodIxSignatureUTF(JNIEnv *env, jclass cb, jint index);

const char*
JVM_GetCPFieldNameUTF(JNIEnv *env, jclass cb, jint index);

const char*
JVM_GetCPMethodNameUTF(JNIEnv *env, jclass cb, jint index);

const char*
JVM_GetCPMethodSignatureUTF(JNIEnv *env, jclass cb, jint index);

const char*
JVM_GetCPFieldSignatureUTF(JNIEnv *env, jclass cb, jint index);

const char*
JVM_GetCPClassNameUTF(JNIEnv *env, jclass cb, jint index);

const char*
JVM_GetCPFieldClassNameUTF(JNIEnv *env, jclass cb, jint index);

const char*
JVM_GetCPMethodClassNameUTF(JNIEnv *env, jclass cb, jint index);

jint
JVM_GetCPFieldModifiers(JNIEnv *env, jclass cb, int index, jclass calledClass);

jint
JVM_GetCPMethodModifiers(JNIEnv *env, jclass cb, int index, jclass calledClass);

void
JVM_ReleaseUTF(const char *utf);

jboolean
JVM_IsSameClassPackage(JNIEnv *env, jclass class1, jclass class2);

jint
JVM_GetLastErrorString(char *buf, int len);

char*
JVM_NativePath(char *);

jint
JVM_Open(const char *fname, jint flags, jint mode);

jint
JVM_Close(jint fd);

jint
JVM_Read(jint fd, char *buf, jint nbytes);

jint
JVM_Write(jint fd, char *buf, jint nbytes);

jint
JVM_Available(jint fd, jlong *pbytes);

jlong
JVM_Lseek(jint fd, jlong offset, jint whence);

jint
JVM_SetLength(jint fd, jlong length);

jint
JVM_Sync(jint fd);

jint
JVM_InitializeSocketLibrary(void);

struct sockaddr;

jint
JVM_Socket(jint domain, jint type, jint protocol);

jint
JVM_SocketClose(jint fd);

jint
JVM_SocketShutdown(jint fd, jint howto);

jint
JVM_Recv(jint fd, char *buf, jint nBytes, jint flags);

jint
JVM_Send(jint fd, char *buf, jint nBytes, jint flags);

jint
JVM_Timeout(int fd, long timeout);

jint
JVM_Listen(jint fd, jint count);

jint
JVM_Connect(jint fd, struct sockaddr *him, jint len);

jint
JVM_Bind(jint fd, struct sockaddr *him, jint len);

jint
JVM_Accept(jint fd, struct sockaddr *him, jint *len);

jint
JVM_RecvFrom(jint fd, char *buf, int nBytes, int flags, struct sockaddr *from, int *fromlen);

jint
JVM_SendTo(jint fd, char *buf, int len, int flags, struct sockaddr *to, int tolen);

jint
JVM_SocketAvailable(jint fd, jint *result);

jint
JVM_GetSockName(jint fd, struct sockaddr *him, int *len);

jint
JVM_GetSockOpt(jint fd, int level, int optname, char *optval, int *optlen);

jint
JVM_SetSockOpt(jint fd, int level, int optname, const char *optval, int optlen);

int
JVM_GetHostName(char* name, int namelen);

int
jio_vsnprintf(char *str, size_t count, const char *fmt, va_list args);

int
jio_snprintf(char *str, size_t count, const char *fmt, ...);

int
jio_fprintf(FILE *, const char *fmt, ...);

int
jio_vfprintf(FILE *, const char *fmt, va_list args);

void*
JVM_RawMonitorCreate(void);

void
JVM_RawMonitorDestroy(void *mon);

jint
JVM_RawMonitorEnter(void *mon);

void
JVM_RawMonitorExit(void *mon);

void*
JVM_GetManagement(jint version);

jobject
JVM_InitAgentProperties(JNIEnv *env, jobject agent_props);

jstring
JVM_GetTemporaryDirectory(JNIEnv *env);

jobjectArray
JVM_GetEnclosingMethodInfo(JNIEnv* env, jclass ofClass);

enum {
    JAVA_THREAD_STATE_NEW           = 0,
    JAVA_THREAD_STATE_RUNNABLE      = 1,
    JAVA_THREAD_STATE_BLOCKED       = 2,
    JAVA_THREAD_STATE_WAITING       = 3,
    JAVA_THREAD_STATE_TIMED_WAITING = 4,
    JAVA_THREAD_STATE_TERMINATED    = 5,
    JAVA_THREAD_STATE_COUNT         = 6
};

jintArray
JVM_GetThreadStateValues(JNIEnv* env, jint javaThreadState);

jobjectArray
JVM_GetThreadStateNames(JNIEnv* env, jint javaThreadState, jintArray values);

typedef struct {
    unsigned int jvm_version;

    unsigned int update_version : 8;
    unsigned int special_update_version : 8;
    unsigned int reserved1 : 16;
    unsigned int reserved2;

    unsigned int is_attach_supported : 1;
    unsigned int is_kernel_jvm : 1;
    unsigned int : 30;
    unsigned int : 32;
    unsigned int : 32;
} jvm_version_info;

void
JVM_GetVersionInfo(JNIEnv* env, jvm_version_info* info, size_t info_size);

void
JVM_GetClassTypeAnnotations();

} // extern "C"
