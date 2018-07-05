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

};

#define LOGI(FORMAT,...) __android_log_print(ANDROID_LOG_INFO,"MusicPlayer",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT,...) __android_log_print(ANDROID_LOG_ERROR,"MusicPlayer",FORMAT,##__VA_ARGS__);


extern "C"
JNIEXPORT void JNICALL
Java_com_fmtech_ffmpegmusic_MusicPlayer_audioToPcm(JNIEnv *env, jobject instance, jstring inputPath_, jstring outputPath_) {
    const char *inputPath = env->GetStringUTFChars(inputPath_, 0);
    const char *outputPath = env->GetStringUTFChars(outputPath_, 0);

    av_register_all();

    AVFormatContext *pFormatCtx = avformat_alloc_context();

    if(avformat_open_input(&pFormatCtx, inputPath, NULL, NULL) != 0){
        LOGE("Open input failed.");
        return;
    }

    if(avformat_find_stream_info(pFormatCtx, NULL) < 0){
        LOGE("Find stream info failed.");
        return;
    }

    int audio_stream_idx = -1;
    int i = 0;
    for(i = 0; i < pFormatCtx->nb_streams; i++){
        if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO){
            audio_stream_idx = i;
            break;
        }
    }

    AVCodecContext *pCodecCtx = pFormatCtx->streams[audio_stream_idx]->codec;
    AVCodec *pCodec = avcodec_find_decoder(pCodecCtx->codec_id);

    if(avcodec_open2(pCodecCtx, pCodec, NULL) < 0){
        LOGE("avcodec_open2 failed.");
        return;
    }

    AVPacket *packet = (AVPacket*)av_malloc(sizeof(AVPacket));
    AVFrame *frame = av_frame_alloc();

    SwrContext *swrContext = swr_alloc();

    int got_frame;

    uint8_t *out_buffer = (uint8_t*)av_malloc(44100 * 2);

    uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;

    //输出采样位数
    enum AVSampleFormat out_sample_format = AV_SAMPLE_FMT_S16;

    //输出采样率必须与输入相同
    int out_sample_rate = pCodecCtx->sample_rate;

    swr_alloc_set_opts(swrContext, out_ch_layout, out_sample_format, out_sample_rate,
                        pCodecCtx->channel_layout, pCodecCtx->sample_fmt, pCodecCtx->sample_rate, 0, NULL);

    swr_init(swrContext);
    int out_channel_nb = av_get_channel_layout_nb_channels(out_ch_layout);
    LOGI("-------Out channecl nb:%d",out_channel_nb);

    FILE *pcm_file = fopen(outputPath, "wb");
    while(av_read_frame(pFormatCtx, packet) >= 0){
        if(packet->stream_index == audio_stream_idx){
            avcodec_decode_audio4(pCodecCtx, frame, &got_frame, packet);
            if(got_frame){
                swr_convert(swrContext, &out_buffer, 44100 * 2, (const uint8_t **) frame->data, frame->nb_samples);
                int out_buffer_size = av_samples_get_buffer_size(NULL, out_channel_nb, frame->nb_samples, out_sample_format, 1);
                fwrite(out_buffer, 1, out_buffer_size, pcm_file);
            }
        }
    }
    LOGI("-------Decode audio success.");

    fclose(pcm_file);
    av_frame_free(&frame);
    av_free(out_buffer);
    swr_free(&swrContext);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);

    env->ReleaseStringUTFChars(inputPath_, inputPath);
    env->ReleaseStringUTFChars(outputPath_, outputPath);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_fmtech_ffmpegmusic_MusicPlayer_playMusic(JNIEnv *env, jobject instance,
                                                  jstring inputPath_) {
    const char *inputPath = env->GetStringUTFChars(inputPath_, 0);

    av_register_all();

    AVFormatContext *pFormatCtx = avformat_alloc_context();

    if(avformat_open_input(&pFormatCtx, inputPath, NULL, NULL) != 0){
        LOGE("Open input failed.");
        return;
    }

    if(avformat_find_stream_info(pFormatCtx, NULL) < 0){
        LOGE("Find stream info failed.");
        return;
    }

    int audio_stream_idx = -1;
    int i = 0;
    for(i = 0; i < pFormatCtx->nb_streams; i++){
        if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO){
            audio_stream_idx = i;
            break;
        }
    }

    AVCodecContext *pCodecCtx = pFormatCtx->streams[audio_stream_idx]->codec;
    AVCodec *pCodec = avcodec_find_decoder(pCodecCtx->codec_id);

    if(avcodec_open2(pCodecCtx, pCodec, NULL) < 0){
        LOGE("avcodec_open2 failed.");
        return;
    }

    AVPacket *packet = (AVPacket*)av_malloc(sizeof(AVPacket));
    AVFrame *frame = av_frame_alloc();

    SwrContext *swrContext = swr_alloc();

    int got_frame;

    uint8_t *out_buffer = (uint8_t*)av_malloc(44100 * 2);

    uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;

    //输出采样位数
    enum AVSampleFormat out_sample_format = AV_SAMPLE_FMT_S16;

    //输出采样率必须与输入相同
    int out_sample_rate = pCodecCtx->sample_rate;

    swr_alloc_set_opts(swrContext, out_ch_layout, out_sample_format, out_sample_rate,
                       pCodecCtx->channel_layout, pCodecCtx->sample_fmt, pCodecCtx->sample_rate, 0, NULL);

    swr_init(swrContext);
    int out_channel_nb = av_get_channel_layout_nb_channels(out_ch_layout);
    LOGI("-------Out channecl nb:%d",out_channel_nb);

    jclass clazzMusicPlayer = env->GetObjectClass(instance);
    jmethodID initAudioTrack = env->GetMethodID(clazzMusicPlayer, "initAudioTrack", "(II)V");
    jmethodID playTrack = env->GetMethodID(clazzMusicPlayer, "playTrack", "([BI)V");
    env->CallVoidMethod(instance, initAudioTrack, 44100, out_channel_nb);

    int frameCount=0;
    while(av_read_frame(pFormatCtx, packet) >= 0){
        if(packet->stream_index == audio_stream_idx){
            avcodec_decode_audio4(pCodecCtx, frame, &got_frame, packet);
            if(got_frame){
                LOGI("Decode %d frame.", frameCount++);
                swr_convert(swrContext, &out_buffer, 44100 * 2, (const uint8_t **) frame->data, frame->nb_samples);
                int out_buffer_size = av_samples_get_buffer_size(NULL, out_channel_nb, frame->nb_samples, out_sample_format, 1);

                jbyteArray audio_sample_array = env->NewByteArray(out_buffer_size);
                env->SetByteArrayRegion(audio_sample_array, 0, out_buffer_size, (const jbyte *) out_buffer);
                env->CallVoidMethod(instance, playTrack, audio_sample_array, out_buffer_size);
                env->DeleteLocalRef(audio_sample_array);
            }
        }
    }
    LOGI("-------Play audio finish.");

    av_frame_free(&frame);
    av_free(out_buffer);
    swr_free(&swrContext);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);

    env->ReleaseStringUTFChars(inputPath_, inputPath);
}