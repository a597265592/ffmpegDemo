#include <jni.h>
#include <string>
#include "CJQueue.h"
#include "utils/Log.h"
#include <android/native_window_jni.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavutil/time.h>
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libswresample/swresample.h>
}

AVFormatContext *avFormatContext;
AVCodecContext *videoContext;
AVCodecContext *audioContext;
bool isStart = false;
int videoIndex = -1;
int audioIndex = -1;
CJQueue *videoQueue;
CJQueue *audioQueue;

AVFrame *rgbFrame;
uint8_t *outbuffer;
SwsContext *swsContext;
SwrContext *swrContext;
jmethodID playTrack;

int width;
int height;
ANativeWindow *nativeWindow;
ANativeWindow_Buffer windowBuffer;

JavaVM *javaVm = NULL;
jobject mInstance;

double singleVideoTime;
double singleAudioTime;

double defaultDelayTime = 0;

double audioNowTime;
double videoNowTime;


double getDelayTime(double diff) {
    double delayTime = 0;
    if (diff > 0.003) {
        delayTime = defaultDelayTime * 2 / 3;
    } else if (diff < -0.003) {
        delayTime = defaultDelayTime * 3/ 2;
    }
    return delayTime;
}

void *decodePacket(void *pvoid) {
    LOGI("解码线程");
    while (isStart) {
        if (audioQueue->size() >= 50 && videoQueue->size() >= 50) {
            continue;
        }
        //avpacket 空容器
        AVPacket *avPacket = av_packet_alloc();
        int ret = av_read_frame(avFormatContext, avPacket);
        if (ret < 0) {
            LOGI("停止解码");
            break;
        }
        //数据放到队列
        if (avPacket->stream_index == videoIndex) {
            videoQueue->push(avPacket);
        } else if (avPacket->stream_index == audioIndex) {
            audioQueue->push(avPacket);
        }
    }
    return NULL;
}

void *decodeVideo(void *pvoid) {
    LOGI("视频读取线程");
    while (isStart) {
        AVPacket *videoPacket = av_packet_alloc();
        videoQueue->getAVPacket(videoPacket);
        //含有压缩数据的 packet对象
        //开始解码
        avcodec_send_packet(videoContext, videoPacket);
        AVFrame *videoFrame = av_frame_alloc();
        int ret = avcodec_receive_frame(videoContext, videoFrame);
        if (ret != 0) {
            LOGI("停止读取,错误信息为%s", av_err2str(ret));
            continue;
        }
        double pts = videoFrame->best_effort_timestamp;
        pts = pts * singleVideoTime;
        videoNowTime = pts;
        double delayTime = getDelayTime(audioNowTime - videoNowTime) * 1000000;
        av_usleep( delayTime);
        sws_scale(swsContext, (const uint8_t *const *) videoFrame->data, videoFrame->linesize, 0,
                  videoContext->height,
                  rgbFrame->data, rgbFrame->linesize);

        ANativeWindow_lock(nativeWindow, &windowBuffer, NULL);
        uint8_t *dstWindow = static_cast<uint8_t *>(windowBuffer.bits);
        for (int i = 0; i < height; ++i) {
            memcpy(dstWindow + i * windowBuffer.stride * 4, outbuffer + i * rgbFrame->linesize[0],
                   rgbFrame->linesize[0]);
        }
        ANativeWindow_unlockAndPost(nativeWindow);
        av_frame_free(&videoFrame);
        av_free(videoFrame);
        videoFrame = NULL;
        av_packet_free(&videoPacket);
        av_free(videoPacket);
        videoPacket = NULL;
    }
    return NULL;
}


void *decodeAudio(void *pvoid) {
    swrContext = swr_alloc();

    uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;
    enum AVSampleFormat out_format = AV_SAMPLE_FMT_S16;
    int out_sample_rate = audioContext->sample_rate;
    swr_alloc_set_opts(swrContext, out_ch_layout, out_format, out_sample_rate,
                       audioContext->channel_layout, audioContext->sample_fmt,
                       audioContext->sample_rate, 0, NULL);
    swr_init(swrContext);

    int out_channer_nb = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
    uint8_t *outbuffer = (uint8_t *) av_malloc(48000 * 2);

    while (isStart) {
        AVPacket *audioPacket = av_packet_alloc();
        audioQueue->getAVPacket(audioPacket);
        //含有压缩数据的 packet对象
        //开始解码
        avcodec_send_packet(audioContext, audioPacket);
        AVFrame *audioFrame = av_frame_alloc();
        int ret = avcodec_receive_frame(audioContext, audioFrame);
        if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
            LOGI("停止读取,错误信息为%s", av_err2str(ret));
            continue;
        }
        if (ret >= 0) {
            //解码之后的大小
            swr_convert(swrContext, &outbuffer, 48000 * 2, (const uint8_t **) (audioFrame->data),
                        audioFrame->nb_samples);
            int size = av_samples_get_buffer_size(NULL, out_channer_nb, audioFrame->nb_samples,
                                                  AV_SAMPLE_FMT_S16, 1);
            audioNowTime = audioFrame->pts * singleAudioTime;

            JNIEnv *jniEnv;
            if (javaVm->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
                continue;
            }
            jbyteArray byteArrays = jniEnv->NewByteArray(size);
            jniEnv->SetByteArrayRegion(byteArrays, 0, size,
                                       reinterpret_cast<const jbyte *>(outbuffer));
            jniEnv->CallVoidMethod(mInstance, playTrack, byteArrays, size);
            jniEnv->DeleteLocalRef(byteArrays);
            javaVm->DetachCurrentThread();
        }
        av_frame_free(&audioFrame);
        av_free(audioFrame);
        audioFrame = NULL;
        av_packet_free(&audioPacket);
        av_free(audioPacket);
        audioPacket = NULL;
    }
}


JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    jint result = -1;
    javaVm = vm;
    JNIEnv *env;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_4) != JNI_OK) {
        return result;
    }
    return JNI_VERSION_1_4;
}

extern "C" JNIEXPORT jstring
Java_com_example_ffmpegdemo_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}


extern "C"
JNIEXPORT void JNICALL
Java_com_example_ffmpegdemo_CJPlayer_play(JNIEnv *env, jobject thiz, jstring url_,
                                          jobject surface) {
    //url转化为char
    const char *url = env->GetStringUTFChars(url_, 0);
    //网络模块初始化
    avformat_network_init();
    //拿到上下文
    avFormatContext = avformat_alloc_context();
    //打开视频文件
    avformat_open_input(&avFormatContext, url, NULL, NULL);
    //视频文件是否可用
    avformat_find_stream_info(avFormatContext, NULL);
    //视频流音频流
    for (int i = 0; i < avFormatContext->nb_streams; i++) {
        AVCodecParameters *avCodecParameters = avFormatContext->streams[i]->codecpar;
        if (avCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoIndex = i;
            //视频流
            LOGI("宽度:%d ", avCodecParameters->width);
            LOGI("高度:%d ", avCodecParameters->height);
            //解码器
            const AVCodec *dec = avcodec_find_decoder(avCodecParameters->codec_id);
            //解码器上下文
            videoContext = avcodec_alloc_context3(dec);
            //设置对应参数
            avcodec_parameters_to_context(videoContext, avCodecParameters);
            //开启解码
            avcodec_open2(videoContext, dec, 0);
            avCodecParameters->video_delay;
            AVRational avRational = avFormatContext->streams[i]->time_base;
            singleVideoTime = avRational.num / (double) avRational.den;
            int num = avFormatContext->streams[i]->avg_frame_rate.num;
            int den = avFormatContext->streams[i]->avg_frame_rate.den;
            double fps = num / (double)den;
            defaultDelayTime = 1.0 / fps;
            LOGI("延时时间defaultDelayTime：%f",defaultDelayTime);
        } else if (avCodecParameters->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioIndex = i;
            //音频流
            //解码器
            const AVCodec *dec1 = avcodec_find_decoder(avCodecParameters->codec_id);
            //解码器上下文
            audioContext = avcodec_alloc_context3(dec1);
            //设置对应参数
            avcodec_parameters_to_context(audioContext, avCodecParameters);
            //开启解码
            avcodec_open2(audioContext, dec1, 0);

            AVRational avRational = avFormatContext->streams[i]->time_base;
            singleAudioTime = avRational.num / (double) avRational.den;
        }
    }
    rgbFrame = av_frame_alloc();

    width = videoContext->width;
    height = videoContext->height;

    mInstance = env->NewGlobalRef(thiz);

    //回调宽高
    jclass player = env->GetObjectClass(thiz);
    jmethodID onSizeChange = env->GetMethodID(player, "onSizeChange", "(II)V");
    env->CallVoidMethod(thiz, onSizeChange, width, height);
    nativeWindow = ANativeWindow_fromSurface(env, surface);

    playTrack = env->GetMethodID(player, "playTrack", "([BI)V");

    jmethodID createTrack = env->GetMethodID(player, "createTrack", "(II)V");
    env->CallVoidMethod(thiz, createTrack, 48000,
                        av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO));

    ANativeWindow_setBuffersGeometry(nativeWindow, width, height, WINDOW_FORMAT_RGBA_8888);


    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_BGRA, width, height, 1);
    outbuffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));

    av_image_fill_arrays(rgbFrame->data, rgbFrame->linesize, outbuffer, AV_PIX_FMT_BGRA, width,
                         height, 1);
    swsContext = sws_getContext(width, height, videoContext->pix_fmt, width, height,
                                AV_PIX_FMT_RGBA,
                                SWS_BICUBIC, NULL, NULL, NULL);

    audioQueue = new CJQueue;
    videoQueue = new CJQueue;
    isStart = true;
    pthread_t thread_decode;
    //读取线程
    pthread_create(&thread_decode, NULL, decodePacket, NULL);
    //视频解码线程
    pthread_t thread_video;
    pthread_create(&thread_video, NULL, decodeVideo, NULL);

    //音频解码线程
    pthread_t thread_audio;
    (&thread_audio, NULL, decodeAudio, NULL);

    //释放
    env->ReleaseStringUTFChars(url_, url);
}
