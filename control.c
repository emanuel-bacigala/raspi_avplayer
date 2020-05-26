#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavresample/avresample.h>
#include <libavutil/opt.h>
#include "key.h"
#include "fifo.h"
#include "cpuload.h"
#include "texturer.h"
#include "control.h"

#define STEP            16
#define ARROW           0x001b5b00
#define CTRL_ARROW      0x313b3500
#define UP      0x41
#define DOWN    0x42
#define RIGHT   0x43
#define LEFT    0x44


int checkKeyPress(appData *userData)
{
    unsigned int key=0;


    if((key = keyboardRead()) > 0)
    {
        //printf("key=0x%08x\n",  key);

        if (key == 27 || key == 113)
        {
            fprintf(stderr, "%s() - Info: shutting down player \n", __FUNCTION__);

            if (userData->haveAudio)
                userData->exitSignal = 4;  // audioThread USER exit signal
            else
                userData->exitSignal = 5;  // videoThread USER exit signal

            return 1;
        }
        else if (key == 115)  // s
        {
            printf("V/A queue %u/%u, render %d/%d, CPU %5.2f%%\n", avpacket_queue_size(&userData->videoPacketFifo),
                   avpacket_queue_size(&userData->audioPacketFifo), fifoGetNumElements(), userData->fifoSize, getCpuLoad());
        }
        else if ((key & 0xffffff00) == ARROW && !TEX_VIEW)
        {
            printf("%s() - Info: render canvas change [%d %d %d %d]", __FUNCTION__,
                   userData->renderWindow[0], userData->renderWindow[1],
                   userData->renderWindow[2], userData->renderWindow[3]);
            if ((key & 0x000000ff) == UP && userData->renderWindow[1] >= STEP)
            {
                userData->renderWindow[1] -= STEP;
            }
            else if ((key & 0x000000ff) == DOWN && userData->renderWindow[1] < userData->dispInfo->height-STEP &&
                     userData->renderWindow[1] < userData->renderWindow[3]-STEP)
            {
                userData->renderWindow[1] += STEP;
            }
            else if ((key & 0x000000ff) == LEFT && userData->renderWindow[0] >= STEP)
            {
                userData->renderWindow[0] -= STEP;
            }
            else if ((key & 0x000000ff) == RIGHT && userData->renderWindow[0] < userData->dispInfo->width-STEP &&
                     userData->renderWindow[0] < userData->renderWindow[2]-STEP &&
                     userData->renderWindow[0] < userData->renderWindow[2]-32)  // min. YUV resource width is 32
            {
                userData->renderWindow[0] += STEP;
            }

            printf(" -> [%d %d %d %d]\n", userData->renderWindow[0], userData->renderWindow[1],
                                        userData->renderWindow[2], userData->renderWindow[3]);
            texturerChangeElement(userData->tex4, userData->renderWindow[0], userData->renderWindow[1],
                                                  userData->renderWindow[2] - userData->renderWindow[0],
                                                  userData->renderWindow[3] - userData->renderWindow[1]);
        }
        else if ((key & 0xffffff00) == CTRL_ARROW && !TEX_VIEW)
        {
            printf("%s() - Info: render canvas change [%d %d %d %d]", __FUNCTION__,
                   userData->renderWindow[0], userData->renderWindow[1],
                   userData->renderWindow[2], userData->renderWindow[3]);

            if ((key & 0x000000ff) == UP && userData->renderWindow[3] > STEP &&
                 userData->renderWindow[3]-STEP > userData->renderWindow[1])
            {
                userData->renderWindow[3] -= STEP;
            }
            else if ((key & 0x000000ff) == DOWN && userData->renderWindow[3] <= userData->dispInfo->height-STEP)
            {
                userData->renderWindow[3] += STEP;
            }
            else if ((key & 0x000000ff) == LEFT && userData->renderWindow[2] > STEP &&
                     userData->renderWindow[2]-STEP > userData->renderWindow[0] &&
                     userData->renderWindow[2]-32   > userData->renderWindow[0])  // min. YUV resource width is 32
            {
                userData->renderWindow[2] -= STEP;
            }
            else if ((key & 0x000000ff) == RIGHT && userData->renderWindow[2] <= userData->dispInfo->width-STEP)
            {
                userData->renderWindow[2] += STEP;
            }

            printf(" -> [%d %d %d %d]\n", userData->renderWindow[0], userData->renderWindow[1],
                                        userData->renderWindow[2], userData->renderWindow[3]);
            texturerChangeElement(userData->tex4, userData->renderWindow[0], userData->renderWindow[1],
                                                  userData->renderWindow[2] - userData->renderWindow[0],
                                                  userData->renderWindow[3] - userData->renderWindow[1]);
        }
        else if (key == 32 )  // space key
        {
            if (userData->exitSignal == 0)
            {
                userData->exitSignal = -1;
                userData->audioPts = 0;   // min. audioPts
                printf("Player paused (warning: pausing LIVE stream may cause connection drop)...\n");
            }
            else if (userData->exitSignal == -1)
            {
                userData->exitSignal = 0;
                userData->audioPts = 0x7FFFFFFFFFFFFFFF;   // max audioPts
                printf("Player resumed...\n");
                printf("V/A queue %u/%u, render %d/%d\n", avpacket_queue_size(&userData->videoPacketFifo),
                   avpacket_queue_size(&userData->audioPacketFifo), fifoGetNumElements(), userData->fifoSize);
            }
        }
    }

    return 0;
}
