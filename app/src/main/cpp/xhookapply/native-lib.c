#include <jni.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <android/log.h>
#include <sys/types.h>
#include <string.h>
#include <malloc.h>
#include "../xhook/xhook.h"

JavaVM *g_VM;

struct func_data {
    void *(*routine)(void *);

    void *arg;
};

void print_stack() {
    JNIEnv *env;
    (*g_VM)->AttachCurrentThread(g_VM, &env, NULL);

    jclass jcl_Log = (*env)->FindClass(env, "android/util/Log");
    jmethodID mid_getStackTraceString = (*env)->GetStaticMethodID(env, jcl_Log, "getStackTraceString", "(Ljava/lang/Throwable;)Ljava/lang/String;");

    jclass cls_Throwable = (*env)->FindClass( env, "java/lang/Throwable");
    jmethodID fn_Throwable = (*env)->GetMethodID(env, cls_Throwable, "<init>", "()V" );
    jobject obj_Throwable = (*env)->NewObject(env, cls_Throwable, fn_Throwable);

    jstring trace = (*env)->CallStaticObjectMethod(env, jcl_Log, mid_getStackTraceString,obj_Throwable);

    const char *trace_str = (*env)->GetStringUTFChars(env, trace, 0);

    (*env)->DeleteLocalRef(env, jcl_Log);
    (*env)->DeleteLocalRef(env, cls_Throwable);
    (*env)->DeleteLocalRef(env, obj_Throwable);

    (*env)->ReleaseStringUTFChars(env, trace, trace_str);
    (*env)->DeleteLocalRef(env,trace);
}

void *hook_start_routine(void *args) {
    __android_log_print(ANDROID_LOG_DEBUG, "DEMO", "pthread started in...tid: %i", gettid());
    // print_stack();
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
