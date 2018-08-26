#include <jni.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <android/log.h>
#include <sys/types.h>
#include <string.h>
#include <malloc.h>
#include "../xhook/xhook.h"


#include <unwind.h>
#include <dlfcn.h>


JavaVM *g_VM;

struct func_data {
    void *(*routine)(void *);

    void *arg;
};

struct BacktraceState {
    intptr_t *current;
    intptr_t *end;
};


static _Unwind_Reason_Code unwindCallback(struct _Unwind_Context *context, void *arg) {
    struct BacktraceState *state = (struct BacktraceState *) (arg);
    intptr_t ip = (intptr_t) _Unwind_GetIP(context);
    if (ip) {
        if (state->current == state->end) {
            return _URC_END_OF_STACK;
        } else {
            state->current[0] = ip;
            state->current++;
        }
    }
    return _URC_NO_REASON;


}

size_t captureBacktrace(intptr_t *buffer, size_t maxStackDeep) {
    struct BacktraceState state = {buffer, buffer + maxStackDeep};
    _Unwind_Backtrace(unwindCallback, &state);
    return state.current - buffer;
}

void dumpBacktraceIndex(char *out, intptr_t *buffer, size_t count) {
    for (size_t idx = 0; idx < count; ++idx) {
        intptr_t addr = buffer[idx];
        const char *symbol = "      ";
        const char *dlfile = "      ";

        Dl_info info;
        if (dladdr((void *) addr, &info)) {
            if (info.dli_sname) {
                symbol = info.dli_sname;
            }
            if (info.dli_fname) {
                dlfile = info.dli_fname;
            }
        } else {
            strcat(out, "#                               \n");
            continue;
        }
        char temp[50];
        memset(temp, 0, sizeof(temp));
        sprintf(temp, "%zu", idx);
        strcat(out, "#");
        strcat(out, temp);
        strcat(out, ": ");
        memset(temp, 0, sizeof(temp));
        sprintf(temp, "%zu", addr);
        strcat(out, temp);
        strcat(out, "  ");
        strcat(out, symbol);
        strcat(out, "      ");
        strcat(out, dlfile);
        strcat(out, "\n");
    }
}

void print_stack_c() {
    const size_t maxStackDeep = 100;
    intptr_t stackBuf[maxStackDeep];
    char outBuf[2048* 100];
    memset(outBuf, 0, sizeof(outBuf));
    dumpBacktraceIndex(outBuf, stackBuf, captureBacktrace(stackBuf, maxStackDeep));
    __android_log_print(ANDROID_LOG_ERROR, "DEMO", " %s\n", outBuf);
}

void print_stack() {
    JNIEnv *env;
    int mNeedDetach = JNI_FALSE;

    //获取当前native线程是否有没有被附加到jvm环境中
    int getEnvStat = (*g_VM)->GetEnv(g_VM, (void **) &env, JNI_VERSION_1_6);
    if (getEnvStat == JNI_EDETACHED) {
        //如果没有， 主动附加到jvm环境中，获取到env
        if ((*g_VM)->AttachCurrentThread(g_VM, &env, NULL) != 0) {
            __android_log_print(ANDROID_LOG_DEBUG, "DEMO", "Fail AttachCurrentThread");
            return;
        }
        mNeedDetach = JNI_TRUE;
    }

    jclass jcl_Log = (*env)->FindClass(env, "android/util/Log");
    jmethodID mid_getStackTraceString = (*env)->GetStaticMethodID(env, jcl_Log,
                                                                  "getStackTraceString",
                                                                  "(Ljava/lang/Throwable;)Ljava/lang/String;");

    jclass cls_Throwable = (*env)->FindClass(env, "java/lang/Throwable");
    jmethodID fn_Throwable = (*env)->GetMethodID(env, cls_Throwable, "<init>", "()V");
    jobject obj_Throwable = (*env)->NewObject(env, cls_Throwable, fn_Throwable);

    jstring trace = (*env)->CallStaticObjectMethod(env, jcl_Log, mid_getStackTraceString,
                                                   obj_Throwable);

    const char *trace_str = (*env)->GetStringUTFChars(env, trace, 0);
    __android_log_print(ANDROID_LOG_ERROR, "DEMO", "FUNCTION = %s TRACE = %s", __FUNCTION__,
                        trace_str);

    (*env)->DeleteLocalRef(env, jcl_Log);
    (*env)->DeleteLocalRef(env, cls_Throwable);
    (*env)->DeleteLocalRef(env, obj_Throwable);

    (*env)->ReleaseStringUTFChars(env, trace, trace_str);
    (*env)->DeleteLocalRef(env, trace);

    //释放当前线程
    if (mNeedDetach) {
        (*g_VM)->DetachCurrentThread(g_VM);
    }
}

void *hook_start_routine(void *args) {
    __android_log_print(ANDROID_LOG_DEBUG, "DEMO", "pthread started in...tid: %i", gettid());
    print_stack();
    print_stack_c();
    struct func_data *data = args;
    return data->routine(data->arg);
}

void *old(void *arg) {
    char *text = arg;
    __android_log_print(ANDROID_LOG_DEBUG, "DEMO", "old func in... %s", text);
    return 0;
}

void start() {
    struct func_data *data = (struct func_data *) malloc(sizeof(struct func_data));
    data->routine = old;
    data->arg = "Hello~";

    hook_start_routine(data);

    pthread_t pth;
    pthread_create(&pth, NULL, hook_start_routine, data);
}

void *hooked_pthread_create(pthread_t *__pthread_ptr, pthread_attr_t const *__attr,
                            void *(*__start_routine)(void *), void *__arg) {
    struct func_data *data = (struct func_data *) malloc(sizeof(struct func_data));
    data->routine = __start_routine;
    data->arg = __arg;
    int res = pthread_create(__pthread_ptr, __attr, hook_start_routine, data);
    return (void *) res;
}

void hook() {
    xhook_register(".*\\.so$", "pthread_create", hooked_pthread_create, NULL);
    xhook_ignore(".*/libxhookapply.so$", "pthread_create");
    xhook_refresh(0);
}

void Java_com_example_haoguo_pthreadtest_XHookApply_start(
        JNIEnv *env, jobject obj) {
    (*env)->GetJavaVM(env, &g_VM);
    hook();
    start();
}
