//
// Created by Drew.Chiang on 2018/7/4.
//

#ifndef FFMPEGOPENSL_FFMPEGMUSIC_H
#define FFMPEGOPENSL_FFMPEGMUSIC_H

#endif //FFMPEGOPENSL_FFMPEGMUSIC_H

#include <jni.h>
#include <string>
#include <android/log.h>
extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
#include <android/native_window_jni.h>
#include <unistd.h>
}
#define LOGI(FORMAT,...) __android_log_print(ANDROID_LOG_INFO,"FMMusicPlayer",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT,...) __android_log_print(ANDROID_LOG_ERROR,"FMMusicPlayer",FORMAT,##__VA_ARGS__);

int initFFmpeg(const char* path, int *sample_rate,int *channels);

int getPcm(void **pcm,size_t *pcm_size);

void releaseFFmpeg();