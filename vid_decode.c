#include "appdata.h"
#include "libavutil/pixdesc.h"
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
    const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(pCodecCtx->pix_fmt);
    AVFrame *pFrame;
    AVPacket pkt;
    int frameFinished;
    int markDeinterlace = 0;
    int64_t pts;
    static int64_t fake_pts=0;
    static int first_packet = 1;

    fprintf(stderr, "%s() - Info: video decoding thread started...\n", __FUNCTION__);

    if ((pFrame = av_frame_alloc()) == NULL)
    {
        fprintf(stderr, "%s() - Error: failed to allocate video frame\n", __FUNCTION__);
        return (void*)1;
    }

    if ((userData->playerState & STATE_FILTERTYPE_MASK)>>STATE_FILTERTYPE_SHIFT > 0 &&
        (userData->playerState & STATE_FILTERTYPE_MASK)>>STATE_FILTERTYPE_SHIFT < 4 )
    {
        markDeinterlace = 1;
        fprintf(stderr, "%s() - Info: marking buffers as interlaced\n", __FUNCTION__);
    }

    while (1)
    {
        if (avpacket_queue_get(&userData->videoPacketFifo, &pkt, 1) == 1)
        {
            if (avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &pkt) < 0 || !frameFinished)
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
                fprintf(stderr, "\tAligned lumma  size: %dx%d\n", pFrame->linesize[0], ALIGN_UP(userData->videoStream->codec->height,16));
                fprintf(stderr, "\tAligned chroma size: %dx%d\n", pFrame->width >> desc->log2_chroma_w, pFrame->height >> desc->log2_chroma_h);
                fprintf(stderr, "%s() - Info: using %d decoding thread(s)\n", __FUNCTION__, pCodecCtx->thread_count);
            }

            videoGetFrame(userData->omxState);

            uint8_t* bufferDataPtr = userData->omxState->video_buf->pBuffer;
            int omxFrameStride     = ALIGN_UP(pFrame->width, 32);
            int omxFrameHeight     = ALIGN_UP(pFrame->height, 16);
            int omxUVPlanesOffset  = (omxFrameHeight/2)*(omxFrameStride >> 1);
            uint8_t* yFrameDataPtr = pFrame->data[0];
            uint8_t* uFrameDataPtr = pFrame->data[1];
            uint8_t* vFrameDataPtr = pFrame->data[2];
            int row;

            for(row = 0; row < omxFrameHeight; row++) { // insert Y component into omx buffer
                memcpy(bufferDataPtr, yFrameDataPtr, pFrame->width);
                bufferDataPtr += omxFrameStride;
                yFrameDataPtr  += pFrame->linesize[0];
            }

            for(row = 0; row < omxFrameHeight/2; row++) { // insert U&V components into omx buffer
                memcpy(bufferDataPtr,                     uFrameDataPtr, pFrame->width >> desc->log2_chroma_w);
                memcpy(bufferDataPtr + omxUVPlanesOffset, vFrameDataPtr, pFrame->width >> desc->log2_chroma_w);
                bufferDataPtr += omxFrameStride >> 1;
                uFrameDataPtr += pFrame->linesize[0] >> desc->log2_chroma_h;
                vFrameDataPtr += pFrame->linesize[0] >> desc->log2_chroma_h;
            }

            userData->omxState->video_buf->nFilledLen = userData->omxState->video_buf->nAllocLen;
            userData->omxState->video_buf->nOffset = 0;
#if (defined(OMX_BUFFERFLAG_INTERLACED) && defined(OMX_BUFFERFLAG_TOP_FIELD_FIRST))
            userData->omxState->video_buf->nFlags = markDeinterlace ? (OMX_BUFFERFLAG_INTERLACED | OMX_BUFFERFLAG_TOP_FIELD_FIRST) : 0;
#else
            userData->omxState->video_buf->nFlags = 0;
#endif
                                                                     //OMX_BUFFERFLAG_TIME_IS_DTS
            if(first_packet)
            {
                userData->omxState->video_buf->nFlags |= OMX_BUFFERFLAG_STARTTIME;
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
