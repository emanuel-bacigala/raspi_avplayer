#include "fifo.h"
#include "texturer.h"
#include "appdata.h"


void vsyncFcn(DISPMANX_UPDATE_HANDLE_T handle, void* params)
{
    appData *userData = (appData*)params;

    if( userData->imageDrawn == 0 && userData->audioPts > userData->videoPts )
    {
        texturerBeginDrawFrame();
        texturerExchangeDisplayDoubleBuffer(userData->tex4);

#if (TEX_VIEW == 1)
        texturerExchangeDisplayDoubleBuffer(userData->tex1);
        texturerExchangeDisplayDoubleBuffer(userData->tex2);
#endif
        texturerEndDrawFrame();

        userData->imageDrawn = 1;
    }
}


void* handleVideoRenderThread(void *params)
{
    appData *userData = (appData*)params;
    buff_element *frame;

    fprintf(stderr, "%s() - Info: video rendering thread started...\n", __FUNCTION__);

    while (!userData->videoParamsSet)
        usleep(1000*200);  // 200ms

// DISPMANX
    userData->dispInfo = texturerInit(0, 255);
    userData->renderWindow[0] = 0;
    userData->renderWindow[1] = 0;
    userData->renderWindow[2] = userData->dispInfo->width;
    userData->renderWindow[3] = userData->dispInfo->height;

    if (TEX_VIEW)
    {
        userData->tex4 = texturerAddResource(userData->videoStream->codec->width | userData->dataWidth << 16,
                                             userData->videoStream->codec->height| userData->dataHeight<< 16,
                                             0, 0,
                                             userData->dispInfo->width/2, userData->dispInfo->height/2, 12);

        userData->tex1 = texturerAddResource(userData->videoStream->codec->width | userData->dataWidth << 16,
                                             userData->videoStream->codec->height| userData->dataHeight<< 16,
                                             0, userData->dispInfo->height/2,
                                             userData->dispInfo->width/2, userData->dispInfo->height/2, 8);

        userData->tex2 = texturerAddResource(userData->videoStream->codec->width/2 | userData->dataWidth/2 << 16,
                                             userData->videoStream->codec->height/1| userData->dataHeight/1<< 16,
                                             userData->dispInfo->width/2, 0,
                                             userData->dispInfo->width/2, userData->dispInfo->height, 8);
    }
    else
    {
        userData->tex4 = texturerAddResource(userData->videoStream->codec->width | userData->dataWidth << 16,
                                             userData->videoStream->codec->height| userData->dataHeight<< 16,
                                             0, 0,
                                             userData->dispInfo->width, userData->dispInfo->height, 12);
    }

    vc_dispmanx_vsync_callback(*(texturerGetDisplayHandle()), (DISPMANX_CALLBACK_FUNC_T) vsyncFcn, userData);
// DISPMANX

    while (userData->exitSignal < 3 || fifoGetNumElements() > 1)
    {
        if ( fifoGetNumElements() > 1 )
        {
            if (userData->imageDrawn == 1)
            {
                frame = fifoGet();

                userData->videoPts = frame->pts;

                texturerSetData(frame->pixBuffer, userData->tex4);

                if (TEX_VIEW)
                {
                    texturerSetData(frame->pixBuffer,                      userData->tex1);
                    texturerSetData(frame->pixBuffer + userData->cbOffset, userData->tex2);
                }

                userData->imageDrawn = 0;
            }
            else
            {
                usleep(10*1000);  // 10ms sleep when drawing incomplete
            }
        }
        else
        {
            usleep(30*1000);  // 30ms sleep
        }

        if (userData->exitSignal == 6)  // renderThread USER exit signal
        {
            break;
        }
    }

    // Close texturer
    vc_dispmanx_vsync_callback(*(texturerGetDisplayHandle()), 0, 0);
    texturerDestroy();

    fprintf(stderr, "%s() - Info: video rendering thread finished\n", __FUNCTION__);

    return 0;
}
