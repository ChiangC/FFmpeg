/**
 *==================================================================
 * Copyright (C) 2018 FMTech All Rights Reserved.
 *
 * @author Drew.Chiang
 * 
 * @email chiangchuna@gmail.com
 * 
 * @create_date 2018/7/14 21:28
 *
 *==================================================================
 */



#ifndef FMPLAYER_FFMPEGVIDEO_H
#define FMPLAYER_FFMPEGVIDEO_H

#include <queue>
#include "FFmpegAudio.h"

extern "C"
{
#include <libavcodec/avcodec.h>

class FFmpegVideo {
public:
    FFmpegVideo();

    ~FFmpegVideo();

    void setAVCodecContext(AVCodecContext *avCodecContext);

    int get(AVPacket *packet);

    int put(AVPacket *packet);

    void play();

    void stop();

    void setPlayCallback(void (*callback)(AVFrame *frame));

    double synchronize(AVFrame *frame, double play);

    void setAudio(FFmpegAudio *audio);

public:
    int isPlay;

    int index;//stream index

    std::queue<AVPacket*> queue;

    pthread_t p_video_tid;

    AVCodecContext *codecCtx;

    pthread_mutex_t mutex;

    pthread_cond_t cond;

    FFmpegAudio *audio;

    AVRational time_base;

    double clock;

};

};


#endif //FMPLAYER_FFMPEGVIDEO_H
