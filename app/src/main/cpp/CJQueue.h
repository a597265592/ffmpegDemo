//
// Created by jiec on 2024/8/12.
//

#ifndef FFMPEGDEMO_CJQUEUE_H
#define FFMPEGDEMO_CJQUEUE_H

#include "queue"
#include "pthread.h"

extern "C" {
#include <libavcodec/avcodec.h>
}

class CJQueue {
public:
    std::queue<AVPacket *> queuePacket;
    pthread_mutex_t mutexPacket;
    pthread_cond_t condPacket;
    bool pkayStatus = false;

public:
    CJQueue();

    ~CJQueue();

    int push(AVPacket *packet);

    int getAVPacket(AVPacket *packet);

    int size();

    void clearAVPacket();
};


#endif //FFMPEGDEMO_CJQUEUE_H
