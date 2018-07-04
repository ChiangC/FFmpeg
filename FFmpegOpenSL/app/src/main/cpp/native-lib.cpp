#include <jni.h>
#include <string>
#include <android/log.h>
extern "C" {
//编码
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
#include <android/native_window_jni.h>
#include <unistd.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

}
#include "FFmpegMusic.h"
#define LOGI(FORMAT,...) __android_log_print(ANDROID_LOG_INFO,"FMMusicPlayer",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT,...) __android_log_print(ANDROID_LOG_ERROR,"FMMusicPlayer",FORMAT,##__VA_ARGS__);

SLObjectItf engineObject = NULL;
SLEngineItf slEngineItf = NULL;
//混音器
SLObjectItf outputMixObject = NULL;
SLEnvironmentalReverbItf outputMixEnvironmentalReverbItf = NULL;
SLEnvironmentalReverbSettings settings = SL_I3DL2_ENVIRONMENT_PRESET_DEFAULT;

extern "C"
JNIEXPORT void JNICALL
Java_com_fmtech_ffmpegopensl_MusicPlayer_play(JNIEnv *env, jobject instance) {
    SLresult sLresult;

    //初始化一个引擎
    slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);

    //获取引擎接口
    (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &slEngineItf);

    //创建混音器
    (*slEngineItf)->CreateOutputMix(slEngineItf, &outputMixObject, 0, 0, 0);
    (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);

    //设置环境混响
    sLresult = (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB,
                                                &outputMixEnvironmentalReverbItf);

    if(SL_RESULT_SUCCESS == sLresult){
        (*outputMixEnvironmentalReverbItf)->SetEnvironmentalReverbProperties(outputMixEnvironmentalReverbItf, &settings);
    }

}


extern "C"
JNIEXPORT void JNICALL
Java_com_fmtech_ffmpegopensl_MusicPlayer_stop(JNIEnv *env, jobject instance) {

    // TODO

}