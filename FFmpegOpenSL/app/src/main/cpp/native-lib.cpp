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
//SLEnvironmentalReverbSettings reverbSettings = SL_I3DL2_ENVIRONMENT_PRESET_DEFAULT;
SLEnvironmentalReverbSettings reverbSettings = SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;

SLObjectItf slPlayerObject;
SLPlayItf  bqPlayerPlay;
//队列缓冲区
SLAndroidSimpleBufferQueueItf  bqPlayerQueue;
//音量对象
SLVolumeItf bqPlayerVolume;

void *buffer;
size_t bufferSize = 0;
void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bufferQueueItf, void *context){
    bufferSize = 0;
    getPcm(&buffer, &bufferSize);
    if(NULL != buffer && 0 != bufferSize){
        SLresult sLresult = (*bqPlayerQueue)->Enqueue(bqPlayerQueue, buffer, bufferSize);
    }
}



extern "C"
JNIEXPORT void JNICALL
Java_com_fmtech_ffmpegopensl_MusicPlayer_play(JNIEnv *env, jobject instance, jstring path_) {
    const char *path = env->GetStringUTFChars(path_, 0);

    SLresult sLresult;

    //初始化一个引擎
    slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    //获取引擎接口
    (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &slEngineItf);


    //创建混音器
    const SLInterfaceID mids[1] = {SL_IID_ENVIRONMENTALREVERB};
    const SLboolean mreq[1] = {SL_BOOLEAN_FALSE};
//    (*slEngineItf)->CreateOutputMix(slEngineItf, &outputMixObject, 0, 0, 0);
    (*slEngineItf)->CreateOutputMix(slEngineItf, &outputMixObject, 1, mids, mreq);
    (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);

    //设置环境混响
    sLresult = (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB,
                                                &outputMixEnvironmentalReverbItf);

    if(SL_RESULT_SUCCESS == sLresult){
        (*outputMixEnvironmentalReverbItf)->SetEnvironmentalReverbProperties(outputMixEnvironmentalReverbItf, &reverbSettings);
    }

    int sample_rate;
    int channels;
    if(initFFmpeg(path, &sample_rate, &channels) < 0){
        LOGE("Init FFmpeg failed.");
        return;
    }


    SLDataLocator_AndroidBufferQueue androidBufferQueue = {SL_DATALOCATOR_ANDROIDBUFFERQUEUE, 2};

    SLDataFormat_PCM slDataFormatPcm = {
            SL_DATAFORMAT_PCM,
            channels,
//            SL_SAMPLINGRATE_44_1,
            SL_SAMPLINGRATE_8,
            SL_PCMSAMPLEFORMAT_FIXED_16,
            SL_PCMSAMPLEFORMAT_FIXED_16,
            SL_SPEAKER_FRONT_LEFT|SL_SPEAKER_FRONT_RIGHT,
            SL_BYTEORDER_LITTLEENDIAN
    };

    SLDataSource slDataSource = {&androidBufferQueue, &slDataFormatPcm};

    //设置混音器
    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};

    SLDataSink slDataSink = {&outputMix, NULL};
    const SLInterfaceID interfaceIDs[3] = {SL_IID_BUFFERQUEUE, SL_IID_EFFECTSEND, SL_IID_VOLUME};
    const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};

    /*关联混音器*/
    //创建一个播放器
    sLresult = (*slEngineItf)->CreateAudioPlayer(slEngineItf, &slPlayerObject, &slDataSource, &slDataSink, 2, interfaceIDs, req);


    if(SL_RESULT_SUCCESS == sLresult && NULL != slPlayerObject){
        LOGI("创建一个播放器成功");
    }else{
        LOGE("CreateAudioPlayer failed");
        return;
    }

    sLresult = (*slPlayerObject)->Realize(slPlayerObject, SL_BOOLEAN_FALSE);
    LOGI("播放器Realize成功");

    //创建播放器接口
    sLresult = (*slPlayerObject)->GetInterface(slPlayerObject, SL_IID_PLAY, &bqPlayerPlay);
    LOGI("创建播放器接口成功");

    //注册缓冲区
    sLresult = (*slPlayerObject)->GetInterface(slPlayerObject, SL_IID_BUFFERQUEUE, &bqPlayerPlay);
    LOGI("注册缓冲区成功");

    //设置回调接口
    sLresult = (*bqPlayerQueue)->RegisterCallback(bqPlayerQueue, bqPlayerCallback, NULL);
    LOGI("设置回调接口成功");

    (*slPlayerObject)->GetInterface(slPlayerObject, SL_IID_VOLUME, &bqPlayerVolume);
    (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);
    //播放第一帧
    bqPlayerCallback(bqPlayerQueue, NULL);
    LOGI("播放第一帧成功");


    env->ReleaseStringUTFChars(path_, path);
}


extern "C"
JNIEXPORT void JNICALL
Java_com_fmtech_ffmpegopensl_MusicPlayer_stop(JNIEnv *env, jobject instance) {

    if(NULL != slPlayerObject){
        (*slPlayerObject)->Destroy(slPlayerObject);
        slPlayerObject = NULL;
        bqPlayerPlay = NULL;
        bqPlayerQueue = NULL;
        bqPlayerVolume = NULL;
    }

    if(NULL != outputMixObject){
        (*outputMixObject)->Destroy(outputMixObject);
        outputMixObject = NULL;
        outputMixEnvironmentalReverbItf = NULL;
    }

    if(NULL != engineObject){
        (*engineObject)->Destroy(engineObject);
        engineObject = NULL;
        slEngineItf = NULL;
    }
    releaseFFmpeg();
}