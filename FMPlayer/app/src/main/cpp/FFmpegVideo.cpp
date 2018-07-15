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

#include "FFmpegVideo.h"

static void (*video_callback)(AVFrame *frame);

FFmpegVideo::FFmpegVideo() {
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
}

FFmpegVideo::~FFmpegVideo() {
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
}

void FFmpegVideo::setAVCodecContext(AVCodecContext *avCodecContext) {
    codecCtx = avCodecContext;
}

int FFmpegVideo::get(AVPacket *packet) {
    pthread_mutex_lock(&mutex);
    while(isPlay){
        if(!queue.empty()){
            if(av_packet_ref(packet, queue.front())){//0 on success, a negative AVERROR on error.
                break;
            }
            AVPacket *pkt = queue.front();
            queue.pop();
            av_free(pkt);
            break;
        }else{
            //If queue is empty then wait;
            pthread_cond_wait(&cond, &mutex);
        }
    }
    pthread_mutex_unlock(&mutex);

    return 0;
}

int FFmpegVideo::put(AVPacket *packet) {
    if(NULL == packet){
        return 0;
    }

    AVPacket *pkt = (AVPacket*)av_mallocz(sizeof(AVPacket));// av_mallocz = av_malloc + memset

    //Copy packet, including contents;0 on success, negative AVERROR on fail
    if(av_copy_packet(pkt, packet)){
        return 0;
    }

    pthread_mutex_lock(&mutex);
    queue.push(pkt);
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
    return 1;
}

void *play_video(void *arg){
    FFmpegVideo *video = (FFmpegVideo*)arg;
    AVFrame *frame = av_frame_alloc();

    SwsContext *sws_ctx = sws_getContext(video->codecCtx->width, video->codecCtx->height,
                                        video->codecCtx->pix_fmt,
                                        video->codecCtx->width, video->codecCtx->height,
                                        AV_PIX_FMT_RGBA, SWS_BILINEAR, NULL, NULL, NULL);

    AVFrame *rgb_frame = av_frame_alloc();
    uint8_t *out_buffer = (uint8_t*)av_malloc(avpicture_get_size(AV_PIX_FMT_RGBA, video->codecCtx->width, video->codecCtx->height));
    /**
    * Setup the picture fields based on the specified image parameters
    * and the provided image data buffer.
    */
    avpicture_fill((AVPicture*)rgb_frame, out_buffer, AV_PIX_FMT_RGBA, video->codecCtx->width, video->codecCtx->height);

    AVPacket *packet = (AVPacket*)av_mallocz(sizeof(AVPacket));

    double last_play,
    play,
    last_delay,
    delay,
    audio_clock,
    diff,
    sync_thresdhold,
    start_time,
    pts,
    actual_delay;
    start_time = av_gettime()/1000000.0;

    int got_frame;
    while(video->isPlay){
        video->get(packet);

        avcodec_decode_video2(video->codecCtx, frame, &got_frame, packet);
        if(!got_frame){
            continue;
        }

        sws_scale(sws_ctx, (const uint8_t *const *)frame->data, frame->linesize, 0, video->codecCtx->height,
                  rgb_frame->data, rgb_frame->linesize);

        if((pts = av_frame_get_best_effort_timestamp(frame)) == AV_NOPTS_VALUE){
            pts = 0;
        }

        play = pts * av_q2d(video->time_base);
        //correct time
        play = video->synchronize(frame, play);
        delay = play - last_play;
        if(delay <= 0 || delay > 1){//1s
            delay = last_delay;
        }
        audio_clock = video->audio->audio_clock;
        last_delay = delay;
        last_play = play;

        diff = video->video_clock - audio_clock;
        sync_thresdhold = (delay > 0.01? 0.01 : delay);

        if(fabs(diff) < 10){
            if(diff <= -sync_thresdhold){
                delay = 0;
            }else if(diff >= sync_thresdhold){
                delay = 2 * delay;
            }
        }

        start_time += delay;
        actual_delay = start_time - av_gettime()/1000000.0;
        if(actual_delay < 0.01){
            actual_delay = 0.01;
        }

        av_usleep(actual_delay * 1000000.0 + 5000);
        video_callback(rgb_frame);

    }

    av_free(packet);
    av_frame_free(&frame);
    av_frame_free(&rgb_frame);
    sws_freeContext(sws_ctx);
    size_t size = video->queue.size();
    for(int i=0; i<size; i++){
        AVPacket *pkt = video->queue.front();
        av_free(pkt);
        video->queue.pop();
    }

    pthread_exit(0);
}

void FFmpegVideo::play() {
    isPlay = 1;
    pthread_create(&p_video_tid, 0, play_video, this);
}

void FFmpegVideo::stop() {
    pthread_mutex_lock(&mutex);
    isPlay = 0;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
    pthread_join(p_video_tid, 0);

    if(this->codecCtx){
        if(avcodec_is_open(this->codecCtx)){
            avcodec_close(codecCtx);
        }
        avcodec_free_context(&this->codecCtx);
        this->codecCtx = NULL;
    }
}

void FFmpegVideo::setPlayCallback(void (*callback)(AVFrame *)) {
    video_callback = callback;
}

double FFmpegVideo::synchronize(AVFrame *frame, double play) {
    if(play != 0){
        video_clock = play;
    }else{
        //pts is 0, set pts as the pts of last frame;
        play = video_clock;
    }

    /**
     * When decoding, this signals how much the picture must be delayed.
     * extra_delay = repeat_pict / (2*fps)
     */
    double repeat_pict = frame->repeat_pict;
    double frame_delay = av_q2d(codecCtx->time_base);//todo
    double fps = 1/frame_delay;
    double extra_delay = repeat_pict/(2*fps);
    double delay = extra_delay + frame_delay;
    video_clock += delay;
    return play;
}

void FFmpegVideo::setAudio(FFmpegAudio *audio) {
    this->audio = audio;
}



