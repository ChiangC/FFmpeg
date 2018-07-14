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

FFmpegVideo::FFmpegVideo() {

}

FFmpegVideo::~FFmpegVideo() {

}

void FFmpegVideo::setAVCodecContext(AVCodecContext *avCodecContext) {

}

int FFmpegVideo::get(AVPacket *packet) {
    return 0;
}

int FFmpegVideo::put(AVPacket *packet) {
    return 0;
}

void FFmpegVideo::play() {

}

void FFmpegVideo::stop() {

}

void FFmpegVideo::setPlayCallback(void (*callback)(AVFrame *)) {

}

double FFmpegVideo::synchronize(AVFrame *frame, double play) {
    return 0;
}

void FFmpegVideo::setAudio(FFmpegAudio *audio) {

}



