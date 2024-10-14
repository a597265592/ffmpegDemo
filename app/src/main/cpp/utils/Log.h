//
// Created by xu on 19-8-14.
//
#include <android/log.h>

#ifndef ARCMEDIA_LOG_H
#define ARCMEDIA_LOG_H


#define TAG __PRETTY_FUNCTION__
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

#endif //ARCMEDIA_LOG_H
