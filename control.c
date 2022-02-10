#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavresample/avresample.h>
#include <libavutil/opt.h>
#include "key.h"
#include "cpuload.h"
#include "control.h"
#include "omx_clock.h"
#include "omx_video.h"

#define STEP            16
#define ARROW           0x001b5b00
#define CTRL_ARROW      0x313b3500
#define SHIFT_ARROW     0x313b3200
#define UP      		0x41
#define DOWN    		0x42
#define RIGHT   		0x43
#define LEFT    		0x44


int checkKeyPress(appData *userData)
{
    static float scale = 1.0;
    static unsigned int filterType = 0;
    unsigned int key=0;


    if((key = keyboardRead()) > 0)
    {
        fprintf(stderr, "%s() - Info: key 0x%08x pressed\n", __FUNCTION__, key);

        if (key == 27 || key == 113)
        {
            fprintf(stderr, "%s() - Info: shutting down player \n", __FUNCTION__);

            userData->playerState &= ~STATE_PAUSED;
            clockSetScale(userData->omxState, 1);
            userData->playerState |= STATE_EXIT;

            return 1;
        }
        else if (key == 115)  // s
        {
            printf("V/A queue %u/%u, CPU %5.2f%%\n", avpacket_queue_size(&userData->videoPacketFifo),
                   avpacket_queue_size(&userData->audioPacketFifo), getCpuLoad());
        }
        else if ((key & 0x00ffff00) == ARROW /*&& !TEX_VIEW*/)
        {
            printf("%s() - Info: render canvas move [%d %d %d %d]", __FUNCTION__,
                   userData->renderWindow[0], userData->renderWindow[1],
                   userData->renderWindow[2], userData->renderWindow[3]);
            if ((key & 0x000000ff) == UP
                 //&& userData->renderWindow[1] >= STEP                            // zmiznutie casti canvas-u hore
                 && userData->renderWindow[1] + userData->renderWindow[3] > STEP // zmiznutie celeho canvas-u hore
               )
            {
                userData->renderWindow[1] -= STEP;
            }
            else if ((key & 0x000000ff) == DOWN
                     //&& userData->renderWindow[1] + userData->renderWindow[3] <= userData->screenHeight-STEP // zmiznutie casti canvas-u
                     && userData->renderWindow[1] < userData->screenHeight-STEP  // zmiznutie celeho canvas-u
                    )
            {
                userData->renderWindow[1] += STEP;
            }
            else if ((key & 0x000000ff) == LEFT
                     //&& userData->renderWindow[0] >= STEP                            // zmiznutie casti canvas-u
                     && userData->renderWindow[0] + userData->renderWindow[2] > STEP // zmiznutie celeho canvas-u
                    )
            {
                userData->renderWindow[0] -= STEP;
            }
            else if ((key & 0x000000ff) == RIGHT
                     //&& userData->renderWindow[0] + userData->renderWindow[2] <= userData->screenWidth-STEP  // zmiznutie celeho canvas-u
                     && userData->renderWindow[0] < userData->screenWidth-STEP  // zmiznutie celeho canvas-u
                    )
            {
                userData->renderWindow[0] += STEP;
            }

            printf(" -> [%d %d %d %d]\n", userData->renderWindow[0], userData->renderWindow[1],
                                        userData->renderWindow[2], userData->renderWindow[3]);

            videoSetDislpayRegion(userData->omxState, userData->renderWindow[0], userData->renderWindow[1],
                                                      userData->renderWindow[2], userData->renderWindow[3]);
        }
        else if ((key & 0xffffff00) == SHIFT_ARROW /*&& !TEX_VIEW*/)
        {
            printf("%s() - Info: render canvas size change [%d %d %d %d]", __FUNCTION__,
                   userData->renderWindow[0], userData->renderWindow[1],
                   userData->renderWindow[2], userData->renderWindow[3]);

            if ((key & 0x000000ff) == UP
                && userData->renderWindow[3] > 32  // min. velkost je 32
                && userData->renderWindow[1] + userData->renderWindow[3] > STEP // zmiznutie
                )
            {
                userData->renderWindow[3] -= STEP;
            }
            else if ((key & 0x000000ff) == DOWN
                     //&& userData->renderWindow[1] + userData->renderWindow[3] <= userData->screenHeight-STEP  // pretecenie za okraj
                    )
            {
                userData->renderWindow[3] += STEP;
            }
            else if ((key & 0x000000ff) == LEFT
                     && userData->renderWindow[2] > 32  // min. velkost je 32
                     && userData->renderWindow[0] + userData->renderWindow[2] > STEP  // zmiznutie
                    )
            {
                userData->renderWindow[2] -= STEP;
            }
            else if ((key & 0x000000ff) == RIGHT
                     //&& userData->renderWindow[0] + userData->renderWindow[2] <= userData->screenWidth-STEP  // pretecenie za okraj
                    )
            {
                userData->renderWindow[2] += STEP;
            }

            printf(" -> [%d %d %d %d]\n", userData->renderWindow[0], userData->renderWindow[1],
                                        userData->renderWindow[2], userData->renderWindow[3]);

            videoSetDislpayRegion(userData->omxState, userData->renderWindow[0], userData->renderWindow[1],
                                                      userData->renderWindow[2], userData->renderWindow[3]);
        }
        else if (key == 32 )  // space key
        {
            if (userData->playerState & STATE_PAUSED)
            {
                userData->playerState &= ~STATE_PAUSED;
                printf("Player resumed...\n");
                printf("V/A queue %u/%u\n", avpacket_queue_size(&userData->videoPacketFifo),
                   avpacket_queue_size(&userData->audioPacketFifo));
                clockSetScale(userData->omxState, scale);
            }
            else
            {
                userData->playerState |= STATE_PAUSED;
                printf("Player paused (warning: pausing LIVE stream may cause connection drop)...\n");
                clockSetScale(userData->omxState, 0);
            }
        }
        else if (key == 0x2d )  // - key
        {
            if (scale > 0.0) scale -= 0.1;
            clockSetScale(userData->omxState, scale);
            printf("%s() - Info: time scale set to %3.2f\n", __FUNCTION__, scale);
            userData->playerState &= ~STATE_PAUSED;
/*
            if(scale < 0.51 && scale > 0.49)
            {
            ilclient_change_component_state(userData->omxState->audio_render, OMX_StateIdle);
            ilclient_change_component_state(userData->omxState->audio_render, OMX_StateLoaded);

            int error;
            OMX_AUDIO_PARAM_PCMMODETYPE pcm;
            OMX_INIT_STRUCTURE(pcm);

            if ((error = OMX_GetParameter(ILC_GET_HANDLE(userData->omxState->audio_render), OMX_IndexParamAudioPcm, &pcm)) != OMX_ErrorNone)
            {
                fprintf(stderr, "%s() - Error: OMX_GetParameter(audio_render, OMX_IndexParamAudioPcm) failed\n", __FUNCTION__);
            }

            pcm.nSamplingRate *= (uint32_t)scale;

            if ((error = OMX_SetParameter(ILC_GET_HANDLE(userData->omxState->audio_render), OMX_IndexParamAudioPcm, &pcm)) != OMX_ErrorNone)
            {
                fprintf(stderr, "%s() - Error: OMX_SetParameter(audio_render, OMX_IndexParamAudioPcm) failed\n", __FUNCTION__);
            }

            ilclient_change_component_state(userData->omxState->audio_render, OMX_StateIdle);
            ilclient_change_component_state(userData->omxState->audio_render, OMX_StateExecuting);
            }
*/
        }
        else if (key == 0x2b )  // + key
        {
            scale += 0.1;
            clockSetScale(userData->omxState, scale);
            printf("%s() - Info: time scale set to %3.2f\n", __FUNCTION__, scale);
            userData->playerState &= ~STATE_PAUSED;
        }
        else if (key == 'm' )
        {
            if (userData->playerState & STATE_MUTE)
            {
                userData->playerState &= ~STATE_MUTE;
                fprintf(stderr, "%s() - Info: audio on\n", __FUNCTION__);
            }
            else
            {
                userData->playerState |= STATE_MUTE;
                fprintf(stderr, "%s() - Info: audio off\n", __FUNCTION__);
            }
        }
        else if (key == 'D')
        {
            videoSetDeinterlace(userData->omxState, ++filterType % 27);
        }
        else if (key == 'd')
        {
            videoSetDeinterlace(userData->omxState, --filterType % 27);
        }
    }

    return 0;
}
