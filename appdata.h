#ifndef _APPDATA_
#define _APPDATA_

#include <stdint.h>
#include <pthread.h>
#include <libavformat/avformat.h>
#include <libavresample/avresample.h>

#include "bcm_host.h"
#include "avqueue.h"

#define INPUT_QUEUE_SIZE        16000000   // 4MB (max. 20MB set in avqueue.c)

#define STATE_HAVEAUDIO		0x00000001
#define STATE_HAVEVIDEO		0x00000002
#define STATE_PAUSED		0x00000004 // play/paused
#define STATE_MUTE		0x00000008
#define STATE_EXIT		0x00000010 // exit request

#define STATE_FILTERTYPE_MASK	0x00000F00 // 15 filters
#define STATE_FILTERTYPE_SHIFT	8

//#define STATE_DISPLAYNO_MASK	0x0000F000 // 15 display numbers available
//#define STATE_DISPLAYNO_SHIFT	8


#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55,28,1)
#define av_frame_alloc  avcodec_alloc_frame
#endif

#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})

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

    uint32_t playerState;  // daj na uint32_t a prepinaj bity [play,paused], [deinterlace], [mute], ...
    uint32_t _playerState;  // daj na uint32_t a prepinaj bity [play,paused], [deinterlace], [mute], ...

    uint32_t displayNo;
    int screenWidth, screenHeight;
    int renderWindow[4];

    char* fileName;
} appData;

#endif
