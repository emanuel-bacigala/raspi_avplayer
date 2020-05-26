#ifndef _APPDATA_
#define _APPDATA_

#include <stdint.h>
#include <pthread.h>
#include <libavformat/avformat.h>
#include <libavresample/avresample.h>

#include "bcm_host.h"
#include "avqueue.h"
#include "audioplay.h"

#define TEX_VIEW                0
#define RING_BUFFER_SIZE        50         // 50 frames for render buffer
#define INPUT_QUEUE_SIZE        16000000   // 16MB (max. 20MB set in avqueue.c)


typedef struct
{
    pthread_t videoThreadId;

    pthread_t videoRenderThreadId;
    int cbOffset;
    int crOffset;
    int videoParamsSet;

    int fifoSize;
    int exitSignal;

    AVPacketQueue videoPacketFifo;
    AVStream *videoStream;

    int haveVideo;

    pthread_t audioThreadId;
    AVPacketQueue audioPacketFifo;
    AVStream *audioStream;
    AVAudioResampleContext *swr;

    int haveAudio;

    DISPMANX_MODEINFO_T *dispInfo;
    uint32_t renderWindow[4];
    int tex1;
    int tex2;
    int tex3;
    int tex4;
    uint32_t dataWidth;
    uint32_t dataHeight;

    AUDIOPLAY_STATE_T *st;

    int64_t audioPts;
    int64_t videoPts;
    int imageDrawn;
} appData;

#endif
