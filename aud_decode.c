#include <assert.h>
#include "appdata.h"

#define BUFFER_SIZE_SAMPLES     8192
#define CTTW_SLEEP_TIME         10
#define MIN_LATENCY_TIME        20


void* handleAudioThread(void *params)
{
    appData *userData = (appData*)params;
    AVCodecContext *pCodecCtx = userData->audioStream->codec;
    AVFrame *pFrame;
    AVPacket pkt;
    int frameFinished;
    int64_t pts;

    int out_linesize;
    uint8_t *buffer;

    int32_t ret;
    uint32_t latency;
    uint8_t *buf;
    int nchannels = 2;
    int bitdepth = 16;
    int buffer_size = (BUFFER_SIZE_SAMPLES * bitdepth * OUT_CHANNELS(nchannels))>>3;
    int filledLen = 0;

    fprintf(stderr, "%s() - Info: audio decoding thread started...\n", __FUNCTION__);
    fprintf(stderr, "%s() - Info: audio buffer size is %d bytes (%d samples) \n", __FUNCTION__, buffer_size, BUFFER_SIZE_SAMPLES);

    if ((ret = audioplay_create(&userData->st, userData->audioStream->codec->sample_rate, nchannels, bitdepth, 10, buffer_size)) != 0)
    {
        fprintf(stderr, "%s() - Error: failed to create audio\n", __FUNCTION__);
        return (void*)1;
    }

    if ((ret = audioplay_set_dest(userData->st, "local")) != 0)
    {
        fprintf(stderr, "%s() - Error: failed to set audio destination\n", __FUNCTION__);
        return (void*)1;
    }

    if ((pFrame = av_frame_alloc()) == NULL)
    {
        fprintf(stderr, "%s() - Error: failed to allocate audio frame\n", __FUNCTION__);
        return (void*)1;
    }

    while ( userData->exitSignal < 1 || avpacket_queue_size(&userData->audioPacketFifo) > 0 )
    {
        if (avpacket_queue_get(&userData->audioPacketFifo, &pkt, 1) == 1)
        {
            if ( avcodec_decode_audio4(pCodecCtx, pFrame, &frameFinished, &pkt) < 0  || !frameFinished )
            {
                av_free_packet(&pkt);
                continue;
            }

            if (pkt.dts != AV_NOPTS_VALUE)
            {
                pts = pkt.dts - userData->videoStream->start_time;
                pts *= 1000*av_q2d(userData->audioStream->time_base); // pts value in [ms]
            }
            else
            {
                pts = 0;
                fprintf(stderr, "%s() - Warning: No audio PTS value.\n", __FUNCTION__);
            }

            userData->audioPts = pts;

            av_samples_alloc(&buffer, &out_linesize, 2, pFrame->nb_samples, AV_SAMPLE_FMT_S16, 0);

            avresample_convert(userData->swr, &buffer,
                                          pFrame->linesize[0],
                                          pFrame->nb_samples,
                                          pFrame->data,
                                          pFrame->linesize[0],
                                          pFrame->nb_samples);

            filledLen = 4*pFrame->nb_samples;

            while((buf = audioplay_get_buffer(userData->st)) == NULL)
                usleep(10*1000);

            if (filledLen > buffer_size)
            {
                fprintf(stderr, "%s() - Error: audio buffer overrun dataLen=%d bufferLen=%d\n", __FUNCTION__, filledLen, buffer_size);
                return (void*)1;
            }

            memcpy(buf, buffer, filledLen);

            while((latency = audioplay_get_latency(userData->st)) > (userData->audioStream->codec->sample_rate * (MIN_LATENCY_TIME + CTTW_SLEEP_TIME) / 1000))
                usleep(CTTW_SLEEP_TIME*1000);

            if ((ret = audioplay_play_buffer(userData->st, buf, filledLen)) != 0)
            {
                fprintf(stderr, "%s() - Error: failed to play audio buffer\n", __FUNCTION__);
                return (void*)1;
            }

            av_free(buffer);
            av_free_packet(&pkt);
        }
        else
        {
            usleep(1000*200);
        }

        while(userData->exitSignal == -1)  // pause player
            usleep(1000*200);

        if (userData->exitSignal == 4)   // audioThread USER exit signal
        {
            break;
        }
    }

    av_free(pFrame);  // free the audio frame

    audioplay_delete(userData->st);
    ilclient_destroy(userData->st->client);

    userData->audioPts = 0x7FFFFFFFFFFFFFFF;   // max audioPts
    userData->exitSignal++;                     // videoThread USER || EOS exit signal
    fprintf(stderr, "%s() - Info: audio decoding thread finished\n", __FUNCTION__);

    return 0;
}
