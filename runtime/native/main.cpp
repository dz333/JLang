// Copyright (C) 2018 Cornell University

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "class.h"
#include "factory.h"
#include "gc.h"
#include "rep.h"
#include "stack_trace.h"
#include "init.h"

extern "C" {

void Polyglot_jlang_runtime_MainWrapper_runMain___3Ljava_lang_String_2(
    jarray args);
jarray Polyglot_jlang_runtime_Factory_ObjectArray__I(jint len);
void Polyglot_java_lang_System_initializeSystemClass__();
} // extern "C"

static void sigaction(int sig, siginfo_t *info, void *ucontext) {
    const char *cause = "";
    if (sig == SIGSEGV)
        cause = "This likely indicates a null pointer exception.\n";
    if (sig == SIGFPE)
        cause = "This indicates an arithmetic exception "
                "(e.g., divide by zero).\n";
    fprintf(stderr,
            "- - - - - - - - - - - - - - - - - - - - - - - - - - -\n"
            "Aborting due to signal: %s\n%s"
            "- - - - - - - - - - - - - - - - - - - - - - - - - - -\n",
            strsignal(sig), cause);
    DumpStackTrace();
    fflush(stderr);
    abort();
}

int main(int argc, char **argv) {
    // Initialize the garbage collector.
    GC_INIT();

    // Set up signal handling to report (for example) null pointer exceptions.
    struct sigaction sa;
    sa.sa_sigaction = sigaction;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGSEGV, &sa, 0);
    sigaction(SIGBUS, &sa, 0);
    sigaction(SIGFPE, &sa, 0);

    RegisterPrimitiveClasses();

    // Ignore the 0th argument, which is the name of the program.
    --argc, ++argv;
    jarray args = Polyglot_jlang_runtime_Factory_ObjectArray__I(argc);
    jstring *args_data = reinterpret_cast<jstring *>(Unwrap(args)->Data());
    for (int i = 0; i < argc; ++i) {
        size_t len = strlen(argv[i]);
        jcharArray argChars = CreateJavaCharArray(len);
        jchar *data = reinterpret_cast<jchar *>(Unwrap(argChars)->Data());
        // TODO: Not a proper character encoding conversion.
        for (size_t j = 0; j < len; ++j)
            data[j] = static_cast<jchar>(argv[i][j]);
        jstring argStr = CreateJavaString(argChars);
        args_data[i] = argStr;
    }
    InitializeMainThread();
    Polyglot_jlang_runtime_MainWrapper_runMain___3Ljava_lang_String_2(args);
}
