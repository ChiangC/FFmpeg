#include <jni.h>
#include <string>
#include <android/log.h>


extern "C"{
#include "libavutil/frame.h"
#include <android/native_window.h>
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include <unistd.h>
#include <libavformat/avformat.h>
#include <android/native_window_jni.h>
}


#define LOGI(FORMAT,...) __android_log_print(ANDROID_LOG_INFO,"FMPlayer",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT,...) __android_log_print(ANDROID_LOG_ERROR,"FMPlayer",FORMAT,##__VA_ARGS__);

extern "C"
JNIEXPORT void JNICALL
Java_com_fmtech_fmplayer_view_VideoView_render(JNIEnv *env, jobject instance, jstring videoUrl_,
                                               jobject surface) {
    const char *videoUrl = env->GetStringUTFChars(videoUrl_, 0);

    av_register_all();

    AVFormatContext *pFormatCtx = avformat_alloc_context();
    if(avformat_open_input(&pFormatCtx, videoUrl, NULL, NULL) != 0){//0 on success, a negative AVERROR on failure.
        LOGE("Open input failed");
        return;
    }

    if(avformat_find_stream_info(pFormatCtx, NULL) < 0){
        LOGE("Get video info failed.");
        return;
    }

    int video_stream_idx = -1;
    int i = 0;
    for(; i < pFormatCtx->nb_streams; i++){
        if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO){
            video_stream_idx = i;
            break;
        }
    }

    AVCodecContext *pCodecCtx = pFormatCtx->streams[video_stream_idx]->codec;
    AVCodec *pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    //Initialize the AVCodecContext to use the given AVCodec.
    if(avcodec_open2(pCodecCtx, pCodec, NULL) < 0){
        LOGE("AVCodec open failed.");
        return;
    }

    AVPacket *packet = (AVPacket*)av_malloc(sizeof(AVPacket));
    AVFrame *frame;
    frame = av_frame_alloc();
    AVFrame *rgb_frame = av_frame_alloc();

    uint8_t  *out_buffer = (uint8_t*)av_malloc(avpicture_get_size(AV_PIX_FMT_RGBA, pCodecCtx->width, pCodecCtx->height));

    //the size in bytes required for src, a negative error code in case of failure
    int memSize = avpicture_fill((AVPicture*)rgb_frame, out_buffer, AV_PIX_FMT_RGBA, pCodecCtx->width, pCodecCtx->height);

    int length = 0;
    int got_frame;

    int frameCount = 0;

    SwsContext *swsContext = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
                                            pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_RGBA, SWS_BICUBIC, NULL, NULL, NULL);

    ANativeWindow *nativeWindow = ANativeWindow_fromSurface(env, surface);
    ANativeWindow_Buffer outBuffer;

    while(av_read_frame(pFormatCtx, packet) >= 0){
        if(packet->stream_index == video_stream_idx){
            length = avcodec_decode_video2(pCodecCtx, frame, &got_frame, packet);

            if(got_frame){
                ANativeWindow_setBuffersGeometry(nativeWindow, pCodecCtx->width, pCodecCtx->height, WINDOW_FORMAT_RGBA_8888);

                ANativeWindow_lock(nativeWindow, &outBuffer, NULL);

                LOGI("Decode %d frame", frameCount++);

                sws_scale(swsContext, (const uint8_t *const *) frame->data, frame->linesize, 0, pCodecCtx->height, rgb_frame->data, rgb_frame->linesize);

                uint8_t *dst = (uint8_t*)outBuffer.bits;
                int destStride = outBuffer.stride*4;
                uint8_t *src = (uint8_t*)rgb_frame->data[0];

                int srcStride = rgb_frame->linesize[0];
                int i =0;
                for(;i < pCodecCtx->height; i++){
                    memcpy(dst + i*destStride, src + i * srcStride, srcStride);
                }
                ANativeWindow_unlockAndPost(nativeWindow);
                usleep(1000 * 16);
            }
        }
        av_free_packet(packet);
    }

    ANativeWindow_release(nativeWindow);
    av_frame_free(&frame);
    av_frame_free(&rgb_frame);
    avcodec_close(pCodecCtx);
    avformat_free_context(pFormatCtx);
    avformat_close_input(&pFormatCtx);

    env->ReleaseStringUTFChars(videoUrl_, videoUrl);
}

//Fatal signal 11 (SIGSEGV), code 1, fault addr 0x58 in tid 32007 (Thread-5)