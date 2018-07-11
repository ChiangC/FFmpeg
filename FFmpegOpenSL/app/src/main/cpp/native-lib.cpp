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
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

}
#include "FFmpegMusic.h"
#define LOGI(FORMAT,...) __android_log_print(ANDROID_LOG_INFO,"FMMusicPlayer",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT,...) __android_log_print(ANDROID_LOG_ERROR,"FMMusicPlayer",FORMAT,##__VA_ARGS__);

SLObjectItf engineObject = NULL;
SLEngineItf engineEngine = NULL;

SLObjectItf outputMixObject = NULL;
SLEnvironmentalReverbItf outputMixEnvironmentalReverbItf = NULL;
//SLEnvironmentalReverbSettings reverbSettings = SL_I3DL2_ENVIRONMENT_PRESET_DEFAULT;
SLEnvironmentalReverbSettings reverbSettings = SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;

SLObjectItf bqPlayerObject;
SLPlayItf  bqPlayerPlay;

SLAndroidSimpleBufferQueueItf  bqPlayerBufferQueue;

SLVolumeItf bqPlayerVolume;

void *buffer;
size_t bufferSize = 0;
void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bufferQueueItf, void *context){
    bufferSize = 0;
    getPcm(&buffer, &bufferSize);
    if(NULL != buffer && 0 != bufferSize){
        SLresult sLresult = (*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, buffer, bufferSize);
    }
}



extern "C"
JNIEXPORT void JNICALL
Java_com_fmtech_ffmpegopensl_MusicPlayer_play(JNIEnv *env, jobject instance, jstring path_) {
    const char *path = env->GetStringUTFChars(path_, 0);

    SLresult sLresult;

    // create engine
    slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    // realize the engine
    (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    // get the engine interface, which is needed in order to create other objects
    (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);


    // create output mix, with environmental reverb specified as a non-required interface
    const SLInterfaceID mids[1] = {SL_IID_ENVIRONMENTALREVERB};
    const SLboolean mreq[1] = {SL_BOOLEAN_FALSE};
    (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 1, mids, mreq);

    // realize the output mix
    (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);

    // get the environmental reverb interface
    // this could fail if the environmental reverb effect is not available,
    // either because the feature is not present, excessive CPU load, or
    // the required MODIFY_AUDIO_SETTINGS permission was not requested and granted
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


    SLDataLocator_AndroidBufferQueue androidBufferQueue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};

    SLDataFormat_PCM slDataFormatPcm = {
            SL_DATAFORMAT_PCM,
            channels,
            SL_SAMPLINGRATE_44_1,//Very important
//            SL_SAMPLINGRATE_8,
            SL_PCMSAMPLEFORMAT_FIXED_16,
            SL_PCMSAMPLEFORMAT_FIXED_16,
            SL_SPEAKER_FRONT_LEFT|SL_SPEAKER_FRONT_RIGHT,
            SL_BYTEORDER_LITTLEENDIAN
    };

    SLDataSource audioSrc = {&androidBufferQueue, &slDataFormatPcm};

    //设置混音器
    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};

    SLDataSink audioSnk = {&outputMix, NULL};
    const SLInterfaceID interfaceIDs[3] = {SL_IID_BUFFERQUEUE, SL_IID_VOLUME, SL_IID_EFFECTSEND};
    const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};

    /*关联混音器*/
    //创建一个播放器
    sLresult = (*engineEngine)->CreateAudioPlayer(engineEngine, &bqPlayerObject, &audioSrc, &audioSnk, 3, interfaceIDs, req);


    if(SL_RESULT_SUCCESS == sLresult && NULL != bqPlayerObject){
        LOGI("CreateAudioPlayer success");
    }else{
        LOGE("CreateAudioPlayer failed");
        return;
    }

    // realize the player
    sLresult = (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);

    // get the play interface
    sLresult = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerPlay);

    // get the buffer queue interface
    sLresult = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE, &bqPlayerBufferQueue);

    // register callback on the buffer queue
    sLresult = (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, bqPlayerCallback, NULL);

    // get the volume interface
    (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_VOLUME, &bqPlayerVolume);
    (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);

    //play
    bqPlayerCallback(bqPlayerBufferQueue, NULL);


    env->ReleaseStringUTFChars(path_, path);
}


extern "C"
JNIEXPORT void JNICALL
Java_com_fmtech_ffmpegopensl_MusicPlayer_stop(JNIEnv *env, jobject instance) {

    if(NULL != bqPlayerObject){
        (*bqPlayerObject)->Destroy(bqPlayerObject);
        bqPlayerObject = NULL;
        bqPlayerPlay = NULL;
        bqPlayerBufferQueue = NULL;
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
        engineEngine = NULL;
    }

    releaseFFmpeg();
}