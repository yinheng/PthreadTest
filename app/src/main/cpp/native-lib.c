#include <jni.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <android/log.h>
#include <sys/types.h>
#include <string.h>
#include <malloc.h>
#include "xhook/xhook.h"

struct func_data {
    void *(*routine)(void *);

    void *arg;
};

void *my(void *args) {
    __android_log_print(ANDROID_LOG_DEBUG, "DEMO", "my func in...%i", gettid());
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

    my(data);

    pthread_t pth;
    pthread_create(&pth, NULL, my, data);
}

void hook() {
    xhook_register("*\\.so$",  "pthread_create", my_system_log_print,  NULL);

    //just for testing
    xhook_ignore(".*/native-lib\\.so$", "pthread_create");
}

void Java_com_example_haoguo_pthreadtest_MainActivity_stringFromJNI(
        JNIEnv *env, jobject obj) {
    __android_log_print(ANDROID_LOG_DEBUG, "DEMO",
                        "Java_com_example_haoguo_pthreadtest_MainActivity_stringFromJNI");
    hook()
    start();
}
