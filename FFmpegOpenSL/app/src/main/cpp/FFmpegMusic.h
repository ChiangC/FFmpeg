//
// Created by 纯 on 2018/7/4.
//

#ifndef FFMPEGOPENSL_FFMPEGMUSIC_H
#define FFMPEGOPENSL_FFMPEGMUSIC_H

#endif //FFMPEGOPENSL_FFMPEGMUSIC_H

#include <jni.h>
#include <string>
#include <android/log.h>
extern "C" {
//编码
#include "libavcodec/avcodec.h"
//封装格式处理
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
//像素处理
#include "libswscale/swscale.h"
#include <android/native_window_jni.h>
#include <unistd.h>
}
#define LOGI(FORMAT,...) __android_log_print(ANDROID_LOG_INFO,"FMMusicPlayer",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT,...) __android_log_print(ANDROID_LOG_ERROR,"FMMusicPlayer",FORMAT,##__VA_ARGS__);

int initFFmpeg(int *rate,int *channel);

int getPcm(void **pcm,size_t *pcm_size);

void realseFFmpeg();