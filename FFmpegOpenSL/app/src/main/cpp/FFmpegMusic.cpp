//
// Created by Drew.Chiang on 2018/7/4.
//

#include "FFmpegMusic.h"

AVFormatContext *pFormatCtx;
AVCodecContext *pCodecCtx;
AVCodec *pCodec;
AVPacket *packet;
AVFrame *frame;
SwrContext *swrContext;
uint8_t *out_buffer;
int out_channel_nb;
int audio_stream_idx = -1;

int initFFmpeg(const char* path, int *sample_rate,int *channels){
    av_register_all();

    pFormatCtx = avformat_alloc_context();

    if(avformat_open_input(&pFormatCtx, path, NULL, NULL) != 0){
        LOGE("Open input failed.");
        return -1;
    }

    if(avformat_find_stream_info(pFormatCtx, NULL) < 0){
        LOGE("Get stream info failed.");
        return -1;
    }

    int i;
    for(i = 0; i < pFormatCtx->nb_streams; i++){
        if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO){
            audio_stream_idx = i;
            break;
        }
    }

    pCodecCtx = pFormatCtx->streams[audio_stream_idx]->codec;
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);

    if(avcodec_open2(pCodecCtx, pCodec, NULL) < 0){
        LOGE("avcodec_open2 failed.");
        return -1;
    }

    packet = (AVPacket*)av_malloc(sizeof(AVPacket));

    frame = av_frame_alloc();

    swrContext = swr_alloc();

    out_buffer = (uint8_t *)av_malloc(44100*2);
    uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;
    enum AVSampleFormat out_format = AV_SAMPLE_FMT_S16;
    int out_sample_rate = pCodecCtx->sample_rate;

    swr_alloc_set_opts(swrContext, out_ch_layout, out_format, out_sample_rate,
                        pCodecCtx->channel_layout, pCodecCtx->sample_fmt, pCodecCtx->sample_rate, 0, NULL);

    swr_init(swrContext);

    out_channel_nb = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
    *sample_rate = pCodecCtx->sample_rate;
    *channels = pCodecCtx->channels;
    return 0;
}

int getPcm(void **pcm,size_t *pcm_size){
    int got_frame;
    while(av_read_frame(pFormatCtx, packet) >= 0){
        if(packet->stream_index == audio_stream_idx){
            avcodec_decode_audio4(pCodecCtx,frame, &got_frame, packet);

            if(got_frame){
                swr_convert(swrContext, &out_buffer, 44100 * 2, (const uint8_t **) frame->data, frame->nb_samples);

                int size = av_samples_get_buffer_size(NULL, out_channel_nb, frame->nb_samples,
                                                      AV_SAMPLE_FMT_S16, 1);

                *pcm = out_buffer;
                *pcm_size = size;
                break;
            }
        }
    }
    return 0;
}

void releaseFFmpeg(){
    av_free_packet(packet);
    av_free(out_buffer);
    av_frame_free(&frame);
    swr_free(&swrContext);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);
}