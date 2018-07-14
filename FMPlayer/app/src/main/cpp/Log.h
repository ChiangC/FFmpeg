/**
 *==================================================================
 * Copyright (C) 2018 FMTech All Rights Reserved.
 *
 * @author Drew.Chiang
 * 
 * @email chiangchuna@gmail.com
 * 
 * @create_date 2018/7/14 21:22
 *
 *==================================================================
 */

#ifndef FMPLAYER_LOG_H
#define FMPLAYER_LOG_H

#include <android/log.h>

#define LOGI(FORMAT,...) __android_log_print(ANDROID_LOG_INFO,"FMPlayer",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT,...) __android_log_print(ANDROID_LOG_ERROR,"FMPlayer",FORMAT,##__VA_ARGS__);

#endif //FMPLAYER_LOG_H
