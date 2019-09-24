#include "init.h"

#include "class.h"
#include "jvm.h"
#include "factory.h"

extern "C" {
void Polyglot_java_lang_System_initializeSystemClass__();
} // extern "C"

static jobject GetMainThread() {
    // Caveat: The name of the main ThreadGroup is "system", which is different
    // from Java's behavior, "main".
    //
    // Even though there is a ThreadGroup(String name) constructor, we could
    // not use it since it assumes that the current thread has already been
    // initialized, which is not the case here. If we really want to follow
    // Java's behavior, we could patch the JDK to have another contructor for 
    // this.

    // create JavaGroup object associated with main Thread.
    auto thread_group_clazz = LoadJavaClassFromLib("java.lang.ThreadGroup");
    jobject mainThreadGroup = CreateJavaObject(thread_group_clazz);
    CallJavaInstanceMethod<jobject>(mainThreadGroup, "<init>", "()V", nullptr);

    // create "main" String object
    const char mainChar[] = "main"; 
    const size_t mainCharLen = 4;
    jcharArray mainCharArray = CreateJavaCharArray(mainCharLen);
    jchar *data = reinterpret_cast<jchar *>(Unwrap(mainCharArray)->Data());
    // TODO: Not a proper character encoding conversion.
    for (size_t i = 0; i < mainCharLen; ++i) {
        data[i] = static_cast<jchar>(mainChar[i]);
    }
    jstring mainStr = CreateJavaString(mainCharArray);

    // create main Thread object.
    auto thread_clazz = GetJavaClassFromName("java.lang.Thread");
    jobject mainThread = CreateJavaObject(thread_clazz);
    const jobject mainThreadArgs[] = {mainThreadGroup, mainStr};
    CallJavaInstanceMethod<jobject>(
        mainThread, "<init>",
        "(Ljava/lang/ThreadGroup;Ljava/lang/String;)V",
        reinterpret_cast<const jvalue *>(mainThreadArgs));
    
    return mainThread;
}

void InitializeMainThread() {
    // initialize classes
    LoadJavaClassFromLib("java.lang.String");
    LoadJavaClassFromLib("java.lang.System");
    LoadJavaClassFromLib("java.lang.Thread");
    LoadJavaClassFromLib("java.lang.ThreadGroup");
    LoadJavaClassFromLib("java.lang.reflect.Method");
    LoadJavaClassFromLib("java.lang.ref.Finalizer");
    LoadJavaClassFromLib("java.lang.Class");

    // setup currentThread for main thread
    currentThread = GetMainThread();
    currentThreadState = JNI_TRUE;

    // initialize the system class
    Polyglot_java_lang_System_initializeSystemClass__();
}