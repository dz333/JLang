// Copyright (C) 2018 Cornell University

#ifndef __JVM_SIGNAL_DEF__
#define __JVM_SIGNAL_DEF__

#include <signal.h>
#include <cstring>

struct siglabel {
    const char *name;
    int number;
};

struct siglabel siglabels[] = {
    /* derived from /usr/include/bits/signum.h on RH7.2 */
    {"HUP", SIGHUP},   /* Hangup (POSIX).  */
    {"INT", SIGINT},   /* Interrupt (ANSI).  */
    {"QUIT", SIGQUIT}, /* Quit (POSIX).  */
    {"ILL", SIGILL},   /* Illegal instruction (ANSI).  */
    {"TRAP", SIGTRAP}, /* Trace trap (POSIX).  */
    {"ABRT", SIGABRT}, /* Abort (ANSI).  */
    {"IOT", SIGIOT},   /* IOT trap (4.2 BSD).  */
    {"BUS", SIGBUS},   /* BUS error (4.2 BSD).  */
    {"FPE", SIGFPE},   /* Floating-point exception (ANSI).  */
    {"KILL", SIGKILL}, /* Kill, unblockable (POSIX).  */
    {"USR1", SIGUSR1}, /* User-defined signal 1 (POSIX).  */
    {"SEGV", SIGSEGV}, /* Segmentation violation (ANSI).  */
    {"USR2", SIGUSR2}, /* User-defined signal 2 (POSIX).  */
    {"PIPE", SIGPIPE}, /* Broken pipe (POSIX).  */
    {"ALRM", SIGALRM}, /* Alarm clock (POSIX).  */
    {"TERM", SIGTERM}, /* Termination (ANSI).  */
#ifdef SIGSTKFLT
    {"STKFLT", SIGSTKFLT}, /* Stack fault.  */
#endif
    {"CHLD", SIGCHLD},     /* Child status has changed (POSIX).  */
    {"CONT", SIGCONT},     /* Continue (POSIX).  */
    {"STOP", SIGSTOP},     /* Stop, unblockable (POSIX).  */
    {"TSTP", SIGTSTP},     /* Keyboard stop (POSIX).  */
    {"TTIN", SIGTTIN},     /* Background read from tty (POSIX).  */
    {"TTOU", SIGTTOU},     /* Background write to tty (POSIX).  */
    {"URG", SIGURG},       /* Urgent condition on socket (4.2 BSD).  */
    {"XCPU", SIGXCPU},     /* CPU limit exceeded (4.2 BSD).  */
    {"XFSZ", SIGXFSZ},     /* File size limit exceeded (4.2 BSD).  */
    {"VTALRM", SIGVTALRM}, /* Virtual alarm clock (4.2 BSD).  */
    {"PROF", SIGPROF},     /* Profiling alarm clock (4.2 BSD).  */
    {"WINCH", SIGWINCH},   /* Window size change (4.3 BSD, Sun).  */
    {"IO", SIGIO},         /* I/O now possible (4.2 BSD).  */
#ifdef SIGSYS
    {"SYS", SIGSYS} /* Bad system call. Only on some Linuxen! */
#endif
};

// TODO NOTE most of this copied from JDK code -> may have to refactor
#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

int FindSignal(const char *name) {
    for (int i = 0; i < ARRAY_SIZE(siglabels); i++) {
        if (!strcmp(name, siglabels[i].name))
            return siglabels[i].number;
    }
    return -1;
}

#endif
