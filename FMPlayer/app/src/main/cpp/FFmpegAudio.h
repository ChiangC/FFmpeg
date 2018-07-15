/**
 *==================================================================
 * Copyright (C) 2018 FMTech All Rights Reserved.
 *
 * @author Drew.Chiang
 * 
 * @email chiangchuna@gmail.com
 * 
 * @create_date 2018/7/14 21:29
 *
 *==================================================================
 */



#ifndef FMPLAYER_FFMPEGAUDIO_H
#define FMPLAYER_FFMPEGAUDIO_H

#include <queue>
#include <SLES/OpenSLES_Android.h>

extern "C"
{
#include "Log.h"
#include <libavcodec/avcodec.h>
#include <pthread.h>
#include <libswresample/swresample.h>

class FFmpegAudio {
public:
    FFmpegAudio();

    ~FFmpegAudio();

    void setAvCodecContext(AVCodecContext *codecContext);

    int get(AVPacket *packet);

    int put(AVPacket *packet);

    void play();

    void stop();

    int createPalyer();

public:
    int isPlay;

    int index;

    std::queue<AVPacket *>queue;

    pthread_t p_audio_tid;

    AVCodecContext *codecCtx;

    pthread_mutex_t mutex;

    pthread_cond_t cond;

    SwrContext *swrContext;

    uint8_t *out_buffer;

    int out_channel_nb;

    double audio_clock;

    AVRational time_base;

    SLObjectItf engineObject;
    SLEngineItf engineEngine;
    SLEnvironmentalReverbItf outputMixEnvironmentalReverb;
    SLObjectItf outputMixObject;
    SLObjectItf bqPlayerObject;
    SLEffectSendItf bqPlayerEffectSend;
    SLVolumeItf bqPlayerVolume;
    SLPlayItf bqPlayerPlay;
    SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue;

};


};

#endif //FMPLAYER_FFMPEGAUDIO_H

