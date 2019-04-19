#include <jni.h>
#include <string>
#include "audio_engine.h"

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_mask_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_mask_MainActivity_record(
        JNIEnv *env,
        jobject /* this */) {
    AudioEngine engine;
    engine.start();
    return;
}