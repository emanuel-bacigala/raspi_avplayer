#include "fifo.h"
#include "appdata.h"


void* handleVideoThread(void *params)
{
    appData *userData = (appData*)params;
    AVCodecContext *pCodecCtx = userData->videoStream->codec;
    AVFrame *pFrame;
    AVPacket pkt;
    int frameFinished;
    int64_t pts;
    buff_element *frame;

    fprintf(stderr, "%s() - Info: video decoding thread started...\n", __FUNCTION__);

    if ((pFrame = av_frame_alloc()) == NULL)
    {
        fprintf(stderr, "%s() - Error: failed to allocate video frame\n", __FUNCTION__);
        return (void*)1;
    }

    while ( userData->exitSignal < 2 || avpacket_queue_size(&userData->videoPacketFifo) > 0 )
    {
        if (avpacket_queue_get(&userData->videoPacketFifo, &pkt, 1) == 1)
        {
            if ( avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &pkt) < 0  || !frameFinished )
            {
                av_free_packet(&pkt);
                continue;
            }

            if (pkt.dts != AV_NOPTS_VALUE)
            {
                pts = pkt.dts - userData->videoStream->start_time;
                pts *= 1000*av_q2d(userData->videoStream->time_base); // value in [ms]
            }
            else
            {
                pts = 0;
                fprintf(stderr, "%s() - Warning: No video PTS value.\n", __FUNCTION__);
            }

            if (userData->videoParamsSet == 0)
            {
                userData->dataWidth = pFrame->linesize[0];   // linesize should be already aligned - works in avcodec >= 54
                userData->dataHeight = ALIGN_UP(userData->videoStream->codec->height,16);
                fprintf(stderr, "%s() - Info: video parameters dump:\n", __FUNCTION__);
                fprintf(stderr, "\tY  component address %p pitch %d\n", (void*)pFrame->data[0], pFrame->linesize[0]);
                fprintf(stderr, "\tCb component address %p pitch %d\n", (void*)pFrame->data[1], pFrame->linesize[1]);
                fprintf(stderr, "\tCr component address %p pitch %d\n", (void*)pFrame->data[2], pFrame->linesize[2]);
                fprintf(stderr, "\tAligned screen size: %dx%d\n", userData->dataWidth, userData->dataHeight);

                userData->cbOffset = userData->dataWidth * userData->dataHeight;
                userData->crOffset = userData->cbOffset + (userData->dataWidth>>1)*(userData->dataHeight>>1);
                fifoInit(userData->fifoSize, 3 * userData->dataWidth * userData->dataHeight * sizeof(unsigned char) / 2);

                fprintf(stderr, "\tAllocated %d elements for video decode ring buffer.\n", userData->fifoSize);
                userData->videoParamsSet = 1;
            }
            else
            {
                while ((frame = fifoPut()) == NULL)
                {
                    usleep(1000*200);  // wait 200ms if renderBuffer full
                }

                memcpy(frame->pixBuffer,                      pFrame->data[0], pFrame->linesize[0] * userData->dataHeight);
                memcpy(frame->pixBuffer + userData->cbOffset, pFrame->data[1], pFrame->linesize[1] * userData->dataHeight/2);
                memcpy(frame->pixBuffer + userData->crOffset, pFrame->data[2], pFrame->linesize[2] * userData->dataHeight/2);
                frame->pts = pts;
            }

            // Free video packet
            av_free_packet(&pkt);
        }
        else
        {
            usleep(1000*200);
        }

        if (userData->exitSignal == 5)  // videoThread USER exit signal
        {
            break;
        }
    }

    // Free the YUV frame
    av_free(pFrame);

    userData->exitSignal++;             // videoRenderThread USER || EOS exit signal
    fprintf(stderr, "%s() - Info: video decoding thread finished\n", __FUNCTION__);

    return 0;
}
