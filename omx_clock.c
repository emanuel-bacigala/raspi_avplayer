#include <stdio.h>
#include "bcm_host.h"
#include "ilclient.h"
#include "omx_clock.h"


int setupClock(omxState_t* omxState)
{
  // create clock
   if(ilclient_create_component(omxState->client, &omxState->clock, "clock", ILCLIENT_DISABLE_ALL_PORTS) != OMX_ErrorNone)
    {
        fprintf(stderr, "%s() - Error: ilclient_create_component(clock)\n", __FUNCTION__);
        return 3;
    }

  // set clock reference
    OMX_TIME_CONFIG_ACTIVEREFCLOCKTYPE rclock;
    OMX_INIT_STRUCTURE(rclock);
    rclock.eClock = OMX_TIME_RefClockAudio;
    //rclock.eClock = OMX_TIME_RefClockVideo;
    if(OMX_SetConfig(ILC_GET_HANDLE(omxState->clock), OMX_IndexConfigTimeActiveRefClock, &rclock) != OMX_ErrorNone)
    {
        fprintf(stderr, "%s() - Error: OMX_SetConfig(clock, OMX_IndexConfigTimeActiveRefClock)\n", __FUNCTION__);
        return 7;
    }

  // set clock start point
    OMX_TIME_CONFIG_CLOCKSTATETYPE cstate;
    OMX_INIT_STRUCTURE(cstate);
    cstate.eState = OMX_TIME_ClockStateWaitingForStartTime;
    //cstate.eState = OMX_TIME_ClockStateRunning;
    //cstate.nWaitMask = OMX_CLOCKPORT0 ;  // if set, pure audio file freezes after start - otherwise ok
    if(OMX_SetParameter(ILC_GET_HANDLE(omxState->clock), OMX_IndexConfigTimeClockState, &cstate) != OMX_ErrorNone)
    {
        fprintf(stderr, "%s() - Error: OMX_SetParameter(clock, OMX_IndexConfigTimeClockState)\n", __FUNCTION__);
        return 4;
    }
/*
  // set clock scale
    OMX_TIME_CONFIG_SCALETYPE scaleType;
    OMX_INIT_STRUCTURE(scaleType);
    scaleType.xScale = 0x00010000;
    //scaleType.xScale = 0x00020000;
    //scaleType.xScale = 0x00008000;

    if(OMX_SetParameter(ILC_GET_HANDLE(omxState->clock), OMX_IndexConfigTimeScale, &scaleType) != OMX_ErrorNone)
    {
        fprintf(stderr, "%s() - Error: OMX_SetParameter() failed\n", __FUNCTION__);
        return 4;
    }
*/

    return 0;
}


int clockSetScale(omxState_t* omxState, float scale)
{
    OMX_TIME_CONFIG_SCALETYPE scaleType;
    OMX_INIT_STRUCTURE(scaleType);
    //scaleType.xScale = state == 2 ? 0x00020000 : state ? 0x00010000 : 0x0;  // double speed works
    //scaleType.xScale = (scale & 0xFF) << 16;
    scaleType.xScale = (uint32_t)(0x00010000 * scale);

    if(OMX_SetParameter(ILC_GET_HANDLE(omxState->clock), OMX_IndexConfigTimeScale, &scaleType) != OMX_ErrorNone)
    {
        fprintf(stderr, "%s() - Error: OMX_SetParameter() failed\n", __FUNCTION__);
        return 1;
    }

    return 0;
}

