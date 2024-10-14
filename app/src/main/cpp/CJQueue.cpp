//
// Created by jiec on 2024/8/12.
//
#include "CJQueue.h"
#include "utils/Log.h"
#include <jni.h>

CJQueue::CJQueue() {
    pthread_mutex_init(&mutexPacket,NULL);
    pthread_cond_init(&condPacket,NULL);
}

CJQueue::~CJQueue() {
    clearAVPacket();
}

int CJQueue::push(AVPacket *packet) {
    pthread_mutex_lock(&mutexPacket);
    queuePacket.push(packet);
    LOGI("当前queuePacket的大小为：%d",queuePacket.size());
    pthread_cond_signal(&condPacket);
    pthread_mutex_unlock(&mutexPacket);
    return 0;
}

int CJQueue::size() {
    int size = 0;
    pthread_mutex_lock(&mutexPacket);
    size = queuePacket.size();
    pthread_mutex_unlock(&mutexPacket);
    return size;
}

int CJQueue::getAVPacket(AVPacket *packet) {
    pthread_mutex_lock(&mutexPacket);
    while (true){
        if (queuePacket.size() >0 ) {
            LOGI("queuePacket的size：%d",queuePacket.size());
            AVPacket *avPacket = queuePacket.front();
            if (av_packet_ref(packet,avPacket) == 0) {
                queuePacket.pop();
            }
            av_packet_free(&avPacket);
            av_free(avPacket);
            break;
        } else {
            LOGI("queuePacket的size小于0");
            pthread_cond_wait(&condPacket,&mutexPacket);
        }
    }
    pthread_mutex_unlock(&mutexPacket);
    return 0;
}

void CJQueue::clearAVPacket() {
    pthread_mutex_lock(&mutexPacket);
    while (!queuePacket.empty()) {
        AVPacket* value = queuePacket.front();
        queuePacket.pop();
        delete value;
    }
    pthread_mutex_unlock(&mutexPacket);
}
