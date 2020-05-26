#include <stdint.h>
#include <time.h>
#include <sys/time.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <libavutil/opt.h>

#include "bcm_host.h"
#include "texturer.h"
#include "avqueue.h"
#include "audioplay.h"
#include "fifo.h"
#include "cpuload.h"
#include "aud_decode.h"
#include "vid_decode.h"
#include "vid_render.h"
#include "control.h"
#include "appdata.h"


#define	AV_CODEC_FLAG2_FAST   		(1 << 0)
#define	AV_CODEC_FLAG_LOW_DELAY		(1 << 19)
#define	AV_CODEC_FLAG_INTERLACED_DCT	(1 << 18)
#define	AV_CODEC_FLAG_INTERLACED_ME	(1 << 29)
#define	AV_CODEC_FLAG_GRAY		(1 << 13)

#ifdef ALIGN_UP
    #undef ALIGN_UP
#endif

#define ALIGN_UP(x,y)  ((x + (y)-1) & ~((y)-1))


int main(int argc, char **argv)
{
    AVFormatContext *pFormatCtx = NULL;
    int i, videoStream, audioStream;
    AVCodec *videoCodec = NULL;
    AVCodec *audioCodec = NULL;
    AVPacket packet;

    AVDictionary *optionsDict = NULL;

    appData *userData = (appData*) malloc(sizeof(appData));
    memset(userData, 0, sizeof(appData));

    userData->haveAudio = 0;
    userData->haveVideo = 0;

    if(argc < 2)
    {
	printf("%s <inFile or stream>\n", argv[0]);
	return -1;
    }

//DEMUXER INIT
    avformat_network_init();

    // Register all formats and codecs
    av_register_all();

    // Open video file
    if(avformat_open_input(&pFormatCtx, argv[1], NULL, NULL) != 0)
    {
        printf("Error: can't open: %s\n", argv[1]);
	return -1;
    }

    // Retrieve stream information
    if(avformat_find_stream_info(pFormatCtx, NULL) < 0)
    {
        printf("Error: can't find stream information\n");
	return -1;
    }

    // Dump information about file onto standard error
    av_dump_format(pFormatCtx, 0, argv[1], 0);
//DEMUXER INIT

// Find the first video stream
    videoStream=-1;
    for(i=0; i < pFormatCtx->nb_streams; i++)
	if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
	    videoStream=i;
            printf("Video stream index: %d\n",videoStream);
	    break;
	}

    if(videoStream==-1)
    {
        printf("Can't find video stream\n");
	return -1;
    }

    userData->videoStream = pFormatCtx->streams[videoStream];

    userData->videoStream->codec->idct_algo = FF_IDCT_SIMPLEARMV6;  // FF_IDCT_SIMPLEARMV6 || FF_IDCT_SIMPLENEON
/*
FF_IDCT_INT            //12.1
FF_IDCT_SIMPLE         //11.9
FF_IDCT_ARM            //11.8
FF_IDCT_SIMPLEARM      //12.1
FF_IDCT_SIMPLEARMV5TE  //11.1
FF_IDCT_SIMPLEARMV6    //10.4
FF_IDCT_SIMPLENEON     //9.0
*/
    //userData->videoStream->codec->flags |= (AV_CODEC_FLAG_INTERLACED_DCT | AV_CODEC_FLAG_INTERLACED_ME | AV_CODEC_FLAG_GRAY);
    //userData->videoStream->codec->flags |= (AV_CODEC_FLAG_LOW_DELAY); |// AV_CODEC_FLAG_INTERLACED_DCT); //| AV_CODEC_FLAG_INTERLACED_ME);  // AV_CODEC_FLAG_GRAY 

    // Find the decoder for the video stream
    videoCodec = avcodec_find_decoder(userData->videoStream->codec->codec_id);

    if(videoCodec==NULL)
    {
	fprintf(stderr, "Unsupported video codec!\n");
	return -1; // Codec not found
    }
    else
    {
        printf("Video codec: %s\n", videoCodec->name);
    }

    // Open video codec
    if(avcodec_open2(userData->videoStream->codec, videoCodec, &optionsDict)<0)
    {
        printf("Could not open video codec\n");
	return -1;
    }

    printf("Video resolution: %dx%d\n", userData->videoStream->codec->width, userData->videoStream->codec->height);

// Find the first video stream

// Find the first audio stream
    audioStream=-1;
    for(i=0; i < pFormatCtx->nb_streams; i++)
	if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
        {
	    audioStream=i;
            printf("Audio stream index: %d\n",audioStream);
	    break;
	}

    if(audioStream==-1)
    {
        printf("Can't find audio stream\n");
    }
    else
    {
        userData->audioStream = pFormatCtx->streams[audioStream];
        //userData->audioStream->codec->flags2 |= AV_CODEC_FLAG2_FAST;  //nema vplyv

        userData->audioStream->codec->idct_algo = FF_IDCT_SIMPLEARMV6;  // FF_IDCT_SIMPLEARMV6 || FF_IDCT_SIMPLENEON

        audioCodec = avcodec_find_decoder(userData->audioStream->codec->codec_id);

        if(audioCodec==NULL)
        {
	    fprintf(stderr, "Unsupported audio codec!\n");
        }
        else
        {
            printf("Audio codec: %s\n", audioCodec->name);

            // Open codec
            if(avcodec_open2(userData->audioStream->codec, audioCodec, NULL) < 0)
            {
                printf("Could not open audio codec\n");
            }
            else
            {
                userData->swr = avresample_alloc_context();
                av_opt_set_int(userData->swr, "in_channel_layout", av_get_default_channel_layout(userData->audioStream->codec->channels), 0);
                av_opt_set_int(userData->swr, "out_channel_layout", AV_CH_LAYOUT_STEREO, 0);
                av_opt_set_int(userData->swr, "in_sample_rate",  userData->audioStream->codec->sample_rate, 0);
                av_opt_set_int(userData->swr, "out_sample_rate", userData->audioStream->codec->sample_rate, 0);
                av_opt_set_int(userData->swr, "in_sample_fmt",   userData->audioStream->codec->sample_fmt, 0);
                av_opt_set_int(userData->swr, "out_sample_fmt",     AV_SAMPLE_FMT_S16, 0);
                avresample_open(userData->swr);

                userData->haveAudio = 1;
            }
        }
    }
// Find the first audio stream

    startCpuLoadDetectionThread();

    avpacket_queue_init(&userData->audioPacketFifo);
    avpacket_queue_init(&userData->videoPacketFifo);

    userData->videoParamsSet = 0;
    userData->exitSignal = 0;
    userData->fifoSize = RING_BUFFER_SIZE;

    bcm_host_init();

    if (userData->haveAudio)
        pthread_create(&userData->audioThreadId, NULL, &handleAudioThread, userData);
    else
        userData->audioPts = 0x7FFFFFFFFFFFFFFF;   // max audioPts

    pthread_create(&userData->videoThreadId, NULL, &handleVideoThread, userData);
    pthread_create(&userData->videoRenderThreadId, NULL, &handleVideoRenderThread, userData);

    keyboardInit();

    while( av_read_frame(pFormatCtx, &packet) >= 0 )
    {
	if(packet.stream_index == videoStream)
        {
            if(avpacket_queue_put(&userData->videoPacketFifo, &packet) != 0)
                goto terminatePlayer;
	}
        else if(packet.stream_index == audioStream)
        {
            if(avpacket_queue_put(&userData->audioPacketFifo, &packet) != 0)
                goto terminatePlayer;
	}
        else  // some other packet type
        {
            // Free the packet that was allocated by av_read_frame
	    av_free_packet(&packet);
        }

        while (avpacket_queue_size(&userData->videoPacketFifo) > INPUT_QUEUE_SIZE)  // wait is input queue full
        {
            if(checkKeyPress(userData))
                goto terminatePlayer;

             usleep(1000*100);
        }

        if(checkKeyPress(userData))
            goto terminatePlayer;
    }

    if (userData->exitSignal == 0)
    {
        printf("Got EOS: V/A queue %u/%u, render %d/%d, CPU %5.2f%%\n", avpacket_queue_size(&userData->videoPacketFifo),
                avpacket_queue_size(&userData->audioPacketFifo), fifoGetNumElements(), userData->fifoSize, getCpuLoad());

        // if EOS wait for threads to finish
        while (avpacket_queue_size(&userData->videoPacketFifo) > 0 || avpacket_queue_size(&userData->audioPacketFifo) > 0)
        {
            usleep(1000*200); // 100ms

            if(checkKeyPress(userData))
                goto terminatePlayer;
        }

        if (userData->haveAudio)
            userData->exitSignal = 1;  // audioThread EOS exit signal
        else
            userData->exitSignal = 2;  // videoThread EOS exit signal

        printf("V/A queue %u/%u, render %d/%d, CPU %5.2f%%\n", avpacket_queue_size(&userData->videoPacketFifo),
                avpacket_queue_size(&userData->audioPacketFifo), fifoGetNumElements(), userData->fifoSize, getCpuLoad());

        printf("exitSignal has been sent to decoding & rendering threads...\n");fflush(stdout);
    }

    terminatePlayer:

    // switch keyboard to normal operation
    keyboardDeinit();

    stopCpuLoadDetectionThread();

    if (userData->haveAudio)
        pthread_join(userData->audioThreadId, NULL);

    pthread_join(userData->videoThreadId, NULL);
    pthread_join(userData->videoRenderThreadId, NULL);

    // Release fifo
    avpacket_queue_release(&userData->videoPacketFifo);
    avpacket_queue_release(&userData->audioPacketFifo);

    // Close the codec
    avcodec_close(userData->videoStream->codec);

    if (userData->haveAudio)
        avcodec_close(userData->audioStream->codec);

    // Close the video file
    avformat_close_input(&pFormatCtx);

    // Release FIFO
    fifoRelease();

    bcm_host_deinit();

    fprintf(stderr, "%s() - Info: player finished\n", __FUNCTION__);

    return 0;
}
