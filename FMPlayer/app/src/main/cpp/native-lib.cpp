#include <jni.h>
#include <string>
#include <android/log.h>
#include "FFmpegAudio.h"
#include "FFmpegVideo.h"

extern "C"{
#include "libavutil/frame.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavformat/avformat.h"
#include <unistd.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>


#define LOGI(FORMAT,...) __android_log_print(ANDROID_LOG_INFO,"FMPlayer",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT,...) __android_log_print(ANDROID_LOG_ERROR,"FMPlayer",FORMAT,##__VA_ARGS__);

ANativeWindow *window;
const char *path;
FFmpegAudio *audio;
FFmpegVideo *video;
pthread_t p_tid;
int isPlay = 0;

void play_video_callback(AVFrame *frame){
    if(!window){
        return;
    }

    ANativeWindow_Buffer window_buffer;
    //0 for success, or a negative value on error.
    if(ANativeWindow_lock(window, &window_buffer, 0)){
        return;
    }

    uint8_t *dst = (uint8_t*)window_buffer.bits;
    int dstStride = window_buffer.stride*4;//
    uint8_t *src = frame->data[0];
    int srcStride = frame->linesize[0];
    for(int i = 0; i < window_buffer.height; i++){
        memcpy(dst + i * dstStride, src + i * srcStride, srcStride);
    }
    ANativeWindow_unlockAndPost(window);
}

void *decode_video_audio(void *args){
    av_register_all();

    avformat_network_init();

    AVFormatContext *pFormatCtx = avformat_alloc_context();

    if(avformat_open_input(&pFormatCtx, path, NULL, NULL) != 0){
        LOGE("Open input failed.");
    }

    if(avformat_find_stream_info(pFormatCtx, NULL) < 0){
        LOGE("Find stream info failed.");
    }

    int i= 0;
    for(; i < pFormatCtx->nb_streams; i++){
        AVCodecContext *pCodecCtx = pFormatCtx->streams[i]->codec;
        AVCodec *pCodec = avcodec_find_decoder(pCodecCtx->codec_id);

        AVCodecContext *avCodecContext = avcodec_alloc_context3(pCodec);
        avcodec_copy_context(avCodecContext, pCodecCtx);
        if(avcodec_open2(avCodecContext, pCodec, NULL) < 0){
            LOGE("Can't open codec.");
            continue;
        }

        if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO){
            video->setAVCodecContext(avCodecContext);
            video->index = i;
            video->time_base = pFormatCtx->streams[i]->time_base;
            if(window){
                ANativeWindow_setBuffersGeometry(window, video->codecCtx->width,
                                        video->codecCtx->height, WINDOW_FORMAT_RGBA_8888);
            }

        }else if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO){
            audio->setAvCodecContext(avCodecContext);
            audio->index = i;
            audio->time_base = pFormatCtx->streams[i]->time_base;
        }
    }

    video->setAudio(audio);
    video->play();
    audio->play();
    isPlay = 1;

    //Decode packet
    AVPacket *packet = (AVPacket *)av_malloc(sizeof(AVPacket));
    int ret;
    while(isPlay){
        ret = av_read_frame(pFormatCtx, packet);
        if(ret == 0){
            if(video && video->isPlay && packet->stream_index == video->index){
                video->put(packet);
            }else if(audio && audio->isPlay && packet->stream_index == audio->index){
                audio->put(packet);
            }
            av_packet_unref(packet);
        }else if(ret == AVERROR_EOF){
            while(isPlay){
                if(video->queue.empty() && audio->queue.empty()){
                    break;
                }
                av_usleep(10000);
            }
        }
    }

    isPlay = 0;
    if(video && video->isPlay){
        video->stop();
    }

    if(audio && audio->isPlay){
        audio->stop();
    }

    av_free_packet(packet);
    avformat_free_context(pFormatCtx);
    pthread_exit(0);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_fmtech_fmplayer_FMPlayer_display(JNIEnv *env, jobject instance, jobject surface) {

    if(window){
        ANativeWindow_release(window);
        window = 0;
    }

    window = ANativeWindow_fromSurface(env, surface);
    if(video && video->codecCtx){
        ANativeWindow_setBuffersGeometry(window, video->codecCtx->width,
                                        video->codecCtx->height, WINDOW_FORMAT_RGBA_8888);
    }

}


extern "C"
JNIEXPORT void JNICALL
Java_com_fmtech_fmplayer_FMPlayer_play(JNIEnv *env, jobject instance, jstring path_) {
    path = env->GetStringUTFChars(path_, 0);

    video = new FFmpegVideo;
    audio = new FFmpegAudio;
    video->setPlayCallback(play_video_callback);
    pthread_create(&p_tid, NULL, decode_video_audio, NULL);
    env->ReleaseStringUTFChars(path_, path);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_fmtech_fmplayer_FMPlayer_release(JNIEnv *env, jobject instance) {

    if(isPlay){
        isPlay = 0;
        pthread_join(p_tid, 0);
    }

    if(video){
        if(video->isPlay){
            video->stop();
        }
        delete(video);
        video = NULL;
    }

    if(audio){
        if(audio->isPlay){
            audio->stop();
        }
        delete(audio);
        audio = NULL;
    }

}

}