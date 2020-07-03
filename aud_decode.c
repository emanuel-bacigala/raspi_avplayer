#include "appdata.h"
#include "omx_audio.h"
#include "aud_decode.h"


static OMX_TICKS ToOMXTime(int64_t pts)
{
    OMX_TICKS ticks;
    ticks.nLowPart = pts;
    ticks.nHighPart = pts >> 32;
    return ticks;
}


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

    int nchannels = 2;
    int bitdepth = 16;
    int buffer_size = (BUFFER_SIZE_SAMPLES * bitdepth * OUT_CHANNELS(nchannels))>>3;
    int filledLen;

    //static int first_packet = 1;

    fprintf(stderr, "%s() - Info: audio decoding thread started...\n", __FUNCTION__);
    fprintf(stderr, "%s() - Info: audio buffer size is %d bytes (%d samples) \n", __FUNCTION__, buffer_size, BUFFER_SIZE_SAMPLES);

    if ((pFrame = av_frame_alloc()) == NULL)
    {
        fprintf(stderr, "%s() - Error: failed to allocate audio frame\n", __FUNCTION__);
        return (void*)1;
    }

    while (1)
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
                if (userData->playerState & STATE_HAVEVIDEO) // musi byt takto, inak stvX dvbT meskal zvuk
                    pts = pkt.dts - userData->videoStream->start_time;
                else
                    pts = pkt.dts - userData->audioStream->start_time;

                pts *= 1000*av_q2d(userData->audioStream->time_base); // pts value in [ms]
                //fprintf(stderr, "Info: a_frame PTS=%llu\n", pts);
            }
            else
            {
                pts = 0;
                fprintf(stderr, "%s() - Warning: No audio PTS value.\n", __FUNCTION__);
            }

            av_samples_alloc(&buffer, &out_linesize, 2, pFrame->nb_samples, AV_SAMPLE_FMT_S16, 0);

            avresample_convert(userData->swr, &buffer,
                                          pFrame->linesize[0],
                                          pFrame->nb_samples,
                                          pFrame->data,
                                          pFrame->linesize[0],
                                          pFrame->nb_samples);

            filledLen = 4*pFrame->nb_samples;

            audioGetFrame(userData->omxState);

            if (filledLen > buffer_size)  // MALO BY PLATIT: buffer_size == userData->omxState->audio_buff->nFilledLen
            {
                fprintf(stderr, "%s() - Error: audio buffer overrun dataLen=%d bufferLen=%d\n", __FUNCTION__, filledLen, buffer_size);
                return (void*)1;
            }

            if (userData->playerState & STATE_MUTE)
                memset(userData->omxState->audio_buf->pBuffer, 0x00, filledLen);
            else
                memcpy(userData->omxState->audio_buf->pBuffer, buffer, filledLen);

            userData->omxState->audio_buf->nFilledLen = filledLen;
            userData->omxState->audio_buf->nTimeStamp = ToOMXTime(1000*pts);  // set PTS in [us]
/*
            userData->omxState->audio_buf->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN;

            if(first_packet)
            {
                userData->omxState->audio_buf->nFlags = OMX_BUFFERFLAG_STARTTIME;  // OMX_BUFFERFLAG_TIME_UNKNOWN
                first_packet = 0;
            }
*/
            while(audioGetLatency(userData->omxState) >
                  (userData->audioStream->codec->sample_rate * (MIN_LATENCY_TIME + CTTW_SLEEP_TIME) / 1000))
                usleep(CTTW_SLEEP_TIME*1000);

            if (audioPutFrame(userData->omxState) != 0)
            {
                fprintf(stderr, "%s() - Error: failed to play audio buffer\n", __FUNCTION__);
                return (void*)1;
            }

            av_free(buffer);
            av_free_packet(&pkt);
        }
        else
        {
            usleep(1000*20);
        }

        while(userData->playerState & STATE_PAUSED)  // pause player
            usleep(1000*20);

        if (userData->playerState & STATE_EXIT)   // audioThread USER exit signal
        {
            //fprintf(stderr, "%s() - Info: STATE_EXIT flag has been set\n", __FUNCTION__);
            break;
        }
    }

    av_free(pFrame);  // free the audio frame

    userData->playerState &= ~STATE_HAVEAUDIO;
    fprintf(stderr, "%s() - Info: audio decoding thread finished\n", __FUNCTION__);

    return 0;
}
