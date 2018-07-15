/**
 *==================================================================
 * Copyright (C) 2018 FMTech All Rights Reserved.
 *
 * @author Drew.Chiang
 * 
 * @email chiangchuna@gmail.com
 * 
 * @create_date 2018/7/14 21:29
 *
 *==================================================================
 */



#include "FFmpegAudio.h"

FFmpegAudio::FFmpegAudio() {
    audio_clock = 0;
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
}

FFmpegAudio::~FFmpegAudio() {
    if(out_buffer){
        free(out_buffer);
    }

    for(int i = 0; i < queue.size(); i++){
        AVPacket *packet = queue.front();
        queue.pop();
        av_free(packet);
    }

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

}

int initAudio(FFmpegAudio *audio){
    audio->swrContext = swr_alloc();

    audio->out_buffer = (uint8_t *)av_malloc(44100 * 2);
    uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;

    enum AVSampleFormat out_format = AV_SAMPLE_FMT_S16;
    int out_sample_rate = audio->codecCtx->sample_rate;

    swr_alloc_set_opts(audio->swrContext, out_ch_layout, out_format, out_sample_rate,
                       audio->codecCtx->channel_layout, audio->codecCtx->sample_fmt,
                       audio->codecCtx->sample_rate, 0, NULL);

    swr_init(audio->swrContext);
    audio->out_channel_nb = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
    return 0;
}

void FFmpegAudio::setAvCodecContext(AVCodecContext *codecContext) {
    codecCtx = codecContext;
    initAudio(this);
}

int FFmpegAudio::get(AVPacket *packet) {
    pthread_mutex_lock(&mutex);
    while(isPlay){
        if(!queue.empty()){
            if(av_packet_ref(packet, queue.front())){
                break;
            }
            AVPacket *pkt = queue.front();
            queue.pop();
            av_free(pkt);
            break;
        }else{
            pthread_cond_wait(&cond, &mutex);
        }

    }
    pthread_mutex_unlock(&mutex);

    return 0;
}

int FFmpegAudio::put(AVPacket *packet) {
    AVPacket *pkt = (AVPacket*)av_mallocz(sizeof(AVPacket));
    if(av_packet_ref(pkt, packet)){
        return 0;
    }

    pthread_mutex_lock(&mutex);
    queue.push(pkt);
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
    return 1;
}

void *play_audio(void *args){
    FFmpegAudio *audio = (FFmpegAudio*)args;
    audio->createPalyer();
    pthread_exit(0);
}

void FFmpegAudio::play() {
    isPlay = 1;
    pthread_create(&p_audio_tid, NULL, play_audio, this);
}

void FFmpegAudio::stop() {
    pthread_mutex_lock(&mutex);
    isPlay = 0;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
    pthread_join(p_audio_tid, 0);

    if(bqPlayerPlay){
        (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_STOPPED);
        bqPlayerPlay = NULL;
    }

    if(bqPlayerObject){
        (*bqPlayerObject)->Destroy(bqPlayerObject);
        bqPlayerObject = NULL;
        bqPlayerBufferQueue = NULL;
        bqPlayerVolume = NULL;
    }

    if(outputMixObject){
        (*outputMixObject)->Destroy(outputMixObject);
        outputMixObject = NULL;
    }

    if(engineObject){
        (*engineObject)->Destroy(engineObject);
        engineObject = NULL;
        engineEngine = NULL;
    }

    if(swrContext){
        swr_free(&swrContext);
    }

    if(this->codecCtx){
        if(avcodec_is_open(this->codecCtx)){
            avcodec_close(this->codecCtx);
        }
        avcodec_free_context(&this->codecCtx);
        this->codecCtx = NULL;
    }
}

int getPcm(FFmpegAudio *audio){
    int got_frame;
    int size;
    AVPacket *packet = (AVPacket*)av_malloc(sizeof(AVPacket));
    AVFrame *frame = av_frame_alloc();

    while(audio->isPlay){
        size = 0;
        audio->get(packet);
        if(packet->pts != AV_NOPTS_VALUE){
            audio->audio_clock = av_q2d(audio->time_base) * packet->pts;
        }

        avcodec_decode_audio4(audio->codecCtx, frame, &got_frame, packet);

        if(got_frame){
            swr_convert(audio->swrContext, &audio->out_buffer, 44100 * 2,
                        (const uint8_t **)frame->data, frame->nb_samples);

            size = av_samples_get_buffer_size(NULL, audio->out_channel_nb, frame->nb_samples, AV_SAMPLE_FMT_S16, 1);
            break;
        }
    }

    av_free(packet);
    av_frame_free(&frame);
    return size;
}

void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context){
    FFmpegAudio *audio = (FFmpegAudio*)context;
    int dataLen = getPcm(audio);
    if(dataLen > 0){
        double time = dataLen/((double)44100*2*2);
        audio->audio_clock = audio->audio_clock + time;
        (*bq)->Enqueue(bq, audio->out_buffer, dataLen);
    }else{
        LOGE("Audio decoding error.")
    }
}

int FFmpegAudio::createPalyer() {
    SLresult lresult;
    lresult = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    if(SL_RESULT_SUCCESS != lresult){
        return 0;
    }

    lresult = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    if(SL_RESULT_SUCCESS != lresult){
        return 0;
    }

    lresult = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
    if(SL_RESULT_SUCCESS != lresult){
        return 0;
    }

    const SLInterfaceID mids[1] = {SL_IID_ENVIRONMENTALREVERB};
    const SLboolean mreq[1] = {SL_BOOLEAN_FALSE};
    lresult = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 1, mids, mreq);//Very important
//    lresult = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 0, 0, 0);//GetInterface (SL_RESULT_FEATURE_UNSUPPORTED)
    if(SL_RESULT_SUCCESS != lresult){
        return 0;
    }

    lresult = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    if(SL_RESULT_SUCCESS != lresult){
        return 0;
    }

    lresult = (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB,
                                                &outputMixEnvironmentalReverb);
    const SLEnvironmentalReverbSettings settings = SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;

    if(SL_RESULT_SUCCESS == lresult){
        (*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(outputMixEnvironmentalReverb, &settings);
    }else{
        return 0;
    }

    SLDataLocator_AndroidBufferQueue android_queue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
    SLDataFormat_PCM pcm = {
            SL_DATAFORMAT_PCM,
            2,
            SL_SAMPLINGRATE_44_1,//Very important
            SL_PCMSAMPLEFORMAT_FIXED_16,
            SL_PCMSAMPLEFORMAT_FIXED_16,
            SL_SPEAKER_FRONT_LEFT|SL_SPEAKER_FRONT_RIGHT,
            SL_BYTEORDER_LITTLEENDIAN
    };

    SLDataSource slDataSource = {&android_queue, &pcm};
    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    SLDataSink audioSink = {&outputMix, NULL};
//    const SLInterfaceID ids[3] = {SL_IID_BUFFERQUEUE, SL_IID_EFFECTSEND, SL_IID_VOLUME};
    const SLInterfaceID ids[3] = {SL_IID_BUFFERQUEUE, SL_IID_VOLUME, SL_IID_EFFECTSEND};
    const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};

    lresult = (*engineEngine)->CreateAudioPlayer(engineEngine, &bqPlayerObject, &slDataSource, &audioSink, 3, ids, req);
    if(SL_RESULT_SUCCESS != lresult){
        LOGE("Create AudioPlayer failed.")
        return 0;
    }

    (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);

    (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerPlay);

    (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE, &bqPlayerBufferQueue);

    (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, bqPlayerCallback, this);

    (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_VOLUME, &bqPlayerVolume);

    (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);

    bqPlayerCallback(bqPlayerBufferQueue, this);
    return 1;
}
