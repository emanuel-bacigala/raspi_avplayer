#ifndef _APPDATA_
#define _APPDATA_

#include <stdint.h>
#include <pthread.h>
#include <libavformat/avformat.h>
#include <libavresample/avresample.h>

#include "bcm_host.h"
#include "avqueue.h"

#define INPUT_QUEUE_SIZE        4000000   // 4MB (max. 20MB set in avqueue.c)

#define STATE_HAVEAUDIO		0x00000001
#define STATE_HAVEVIDEO		0x00000002
#define STATE_PAUSED		0x00000004 // play/paused
#define STATE_EXIT		0x00000008 // exit request
#define STATE_DEINTERLACE	0x00000010
#define STATE_MUTE		0x00000020

struct omxState_t;

typedef struct
{
    struct omxState_t* omxState;

    pthread_t videoThreadId;
    AVPacketQueue videoPacketFifo;
    AVStream *videoStream;

    pthread_t audioThreadId;
    AVPacketQueue audioPacketFifo;
    AVStream *audioStream;
    AVAudioResampleContext *swr;

    int playerState;  // daj na uint32_t a prepinaj bity [play,paused], [deinterlace], [mute], ...
} appData;

#endif
