#include "appdata.h"
#include "omx_video.h"


static OMX_TICKS ToOMXTime(int64_t pts)
{
    OMX_TICKS ticks;
    ticks.nLowPart = pts;
    ticks.nHighPart = pts >> 32;
    return ticks;
}



void* handleVideoThread(void *params)
{
    appData *userData = (appData*)params;
    AVCodecContext *pCodecCtx = userData->videoStream->codec;
    AVFrame *pFrame;
    AVPacket pkt;
    int frameFinished;
    int64_t pts;
    static int64_t fake_pts=0;
    static int first_packet = 1;

    fprintf(stderr, "%s() - Info: video decoding thread started...\n", __FUNCTION__);

    if ((pFrame = av_frame_alloc()) == NULL)
    {
        fprintf(stderr, "%s() - Error: failed to allocate video frame\n", __FUNCTION__);
        return (void*)1;
    }

    //while (avpacket_queue_size(&userData->videoPacketFifo) > 0)
    while (1)
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
                //fprintf(stderr, "Info: v_frame PTS=%llu\n", pts);
            }
            else
            {
                if (first_packet)
                    fprintf(stderr, "%s() - Warning: No video PTS value.\n", __FUNCTION__);

                pts = 0;
            }

            if (first_packet)
            {
                fprintf(stderr, "%s() - Info: video parameters dump:\n", __FUNCTION__);
                fprintf(stderr, "\tY  component address %p pitch %d\n", (void*)pFrame->data[0], pFrame->linesize[0]);
                fprintf(stderr, "\tCb component address %p pitch %d\n", (void*)pFrame->data[1], pFrame->linesize[1]);
                fprintf(stderr, "\tCr component address %p pitch %d\n", (void*)pFrame->data[2], pFrame->linesize[2]);
// TREBA POUZIVAT ALIGN_UP ???
                fprintf(stderr, "\tAligned video size: %dx%d\n", pFrame->linesize[0], ALIGN_UP(userData->videoStream->codec->height,16));
            }

            videoGetFrame(userData->omxState);

            uint8_t* bufferDataPtr  = userData->omxState->video_buf->pBuffer;
            int renderFrameStride   = ALIGN_UP(userData->videoStream->codec->width, 32);
            uint8_t* frameDataPtr   = pFrame->data[0];
            int libavFrameStride    = pFrame->linesize[0];
            int frameWidth          = userData->videoStream->codec->width;
            int frameHeight         = ALIGN_UP(userData->videoStream->codec->height, 16);
            int row;

            for(row=0; row<frameHeight; row++)  // insert Y component
            {
                memcpy(bufferDataPtr, frameDataPtr, frameWidth);
                bufferDataPtr += renderFrameStride;
                frameDataPtr  += libavFrameStride;
            }

            frameDataPtr = pFrame->data[1];
            libavFrameStride = pFrame->linesize[1];
            for(row=0; row<frameHeight/2; row++)  // insert U component
            {
                memcpy(bufferDataPtr, frameDataPtr, frameWidth/2);
                bufferDataPtr += renderFrameStride/2;
                frameDataPtr  += libavFrameStride;
            }

            frameDataPtr = pFrame->data[2];
            libavFrameStride = pFrame->linesize[2];
            for(row=0; row<frameHeight/2; row++)  // insert V component
            {
                memcpy(bufferDataPtr, frameDataPtr, frameWidth/2);
                bufferDataPtr += renderFrameStride/2;
                frameDataPtr  += libavFrameStride;
            }

            userData->omxState->video_buf->nFilledLen = userData->omxState->video_buf->nAllocLen;
            userData->omxState->video_buf->nOffset = 0;
            userData->omxState->video_buf->nFlags = 0;

            if(first_packet)
            {
                userData->omxState->video_buf->nFlags = OMX_BUFFERFLAG_STARTTIME;
                first_packet = 0;
            }
            else
            {
                if (pts == 0)
                {
                    userData->omxState->video_buf->nTimeStamp = ToOMXTime(1000*fake_pts);  // value in [us]
                    fake_pts += 33;  // force 30 FPS
                }
                else
                    userData->omxState->video_buf->nTimeStamp = ToOMXTime(1000*pts);  // value in [us]
            }

            videoPutFrame(userData->omxState);

            // Free video packet
            av_free_packet(&pkt);
        }
        else
        {
            usleep(1000*20);
        }

        if ((userData->playerState & STATE_EXIT) && !(userData->playerState & STATE_HAVEAUDIO))  // videoThread user_exit + audio_finished
        {
            //fprintf(stderr, "%s() - Info: STATE_EXIT flag has been set\n", __FUNCTION__);
            break;
        }
    }

    // Free the YUV frame
    av_free(pFrame);

    userData->playerState &= ~STATE_HAVEVIDEO;
    fprintf(stderr, "%s() - Info: video decoding thread finished\n", __FUNCTION__);

    return 0;
}
