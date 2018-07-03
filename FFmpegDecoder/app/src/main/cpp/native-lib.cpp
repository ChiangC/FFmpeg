#include <jni.h>
#include <string>
#include <android/log.h>

extern "C"{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
}

#define LOGI(FORMAT, ...) __android_log_print(ANDROID_LOG_INFO, "FFmpegDecoder", FORMAT, ##__VA_ARGS__);
#define LOGE(FORMAT, ...) __android_log_print(ANDROID_LOG_ERROR, "FFmpegDecoder", FORMAT, ##__VA_ARGS__);

extern "C"
JNIEXPORT void JNICALL
Java_com_fmtech_ffmpegdecoder_MainActivity_mp4ToYuv(JNIEnv *env, jobject instance,
                                                    jstring inputPath_, jstring outPutPath_) {
    const char *inputPath = env->GetStringUTFChars(inputPath_, 0);
    const char *outPutPath = env->GetStringUTFChars(outPutPath_, 0);

    av_register_all();

    AVFormatContext *pContext = avformat_alloc_context();

    //Open an input stream and read the header. The codecs are not opened.
    if(avformat_open_input(&pContext, inputPath, NULL, NULL) < 0){
        LOGE("Open input failed.");
        return;
    }


    //Read packets of a media file to get stream information.
    if(avformat_find_stream_info(pContext, NULL) < 0){
        LOGE("Get stream information failed.");
        return;
    }

    int video_stream_idx = -1;
    //Find video stream
    for(int i=0; i < pContext->nb_streams; i++){
        if(pContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO){
            video_stream_idx = i;
        }
    }

    AVCodecContext *pCodecCtx = pContext->streams[video_stream_idx]->codec;

    AVCodec *pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if(avcodec_open2(pCodecCtx, pCodec, NULL) < 0){
        LOGE("avcodec open failed.");
        return;
    }

    AVPacket *packet = (AVPacket*)av_malloc(sizeof(AVPacket));
    av_init_packet(packet);

    AVFrame *frame = av_frame_alloc();
    AVFrame *yuvFrame = av_frame_alloc();

    uint8_t *out_buffer = (uint8_t *)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height));

    /**
    * Setup the picture fields based on the specified image parameters
    * and the provided image data buffer.
    */
    int re = avpicture_fill((AVPicture*)yuvFrame, out_buffer, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);

    SwsContext *swsContext = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
                                            pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P,
                                            SWS_BILINEAR, NULL, NULL, NULL);

    int frameCount = 0;
    FILE *fp_yuv = fopen(outPutPath, "wb");

    int got_frame;
    while(av_read_frame(pContext, packet) >= 0){
        avcodec_decode_video2(pCodecCtx, frame, &got_frame, packet);

        if(got_frame > 0){
            /**
            * Scale the image slice in srcSlice and put the resulting scaled
            * slice in the image in dst. A slice is a sequence of consecutive
            * rows in an image.
            */
            sws_scale(swsContext, (const uint8_t *const *) frame->data, frame->linesize, 0, frame->height, yuvFrame->data, yuvFrame->linesize);
            int y_size = pCodecCtx->width * pCodecCtx->height;
            fwrite(yuvFrame->data[0], 1, y_size, fp_yuv);
            fwrite(yuvFrame->data[1], 1, y_size/4, fp_yuv);
            fwrite(yuvFrame->data[2], 1, y_size/4, fp_yuv);
        }

        av_free_packet(packet);
    }

    fclose(fp_yuv);
    av_frame_free(&frame);
    av_frame_free(&yuvFrame);
    avcodec_close(pCodecCtx);
    avformat_free_context(pContext);

    env->ReleaseStringUTFChars(inputPath_, inputPath);
    env->ReleaseStringUTFChars(outPutPath_, outPutPath);
}