#include <stdio.h>
#include "omx_integration.h"
#include "omx_clock.h"
#include "omx_video.h"
#include "omx_audio.h"

/*
 * -1: a timeout waiting for the parameter changed
 * -2: an error was returned instead of parameter changed
 * -3: no streams are available from this port
 * -4: requested stream is not available from this port
 * -5: the data format was not acceptable to the sink
*/

const char* dump_tunnel_err(int error){
    switch (error)
    {
        case 0:
            return "success";
        case -1:
            return "timeout waiting for the parameter changed";
        case -2:
            return "error returned instead of parameter changed";
        case -3:
            return "no streams available from port";
        case -4:
            return "requested stream not available from port";
        case -5:
            return "data format not acceptable to the sink";
        default:
            return "unknown error type";
    }
}


#define DUMP_CASE(x) case x: return #x;

const char* dump_OMX_ERRORTYPE (OMX_ERRORTYPE error){
  switch (error){
    DUMP_CASE (OMX_ErrorNone)
    DUMP_CASE (OMX_ErrorInsufficientResources)
    DUMP_CASE (OMX_ErrorUndefined)
    DUMP_CASE (OMX_ErrorInvalidComponentName)
    DUMP_CASE (OMX_ErrorComponentNotFound)
    DUMP_CASE (OMX_ErrorInvalidComponent)
    DUMP_CASE (OMX_ErrorBadParameter)
    DUMP_CASE (OMX_ErrorNotImplemented)
    DUMP_CASE (OMX_ErrorUnderflow)
    DUMP_CASE (OMX_ErrorOverflow)
    DUMP_CASE (OMX_ErrorHardware)
    DUMP_CASE (OMX_ErrorInvalidState)
    DUMP_CASE (OMX_ErrorStreamCorrupt)
    DUMP_CASE (OMX_ErrorPortsNotCompatible)
    DUMP_CASE (OMX_ErrorResourcesLost)
    DUMP_CASE (OMX_ErrorNoMore)
    DUMP_CASE (OMX_ErrorVersionMismatch)
    DUMP_CASE (OMX_ErrorNotReady)
    DUMP_CASE (OMX_ErrorTimeout)
    DUMP_CASE (OMX_ErrorSameState)
    DUMP_CASE (OMX_ErrorResourcesPreempted)
    DUMP_CASE (OMX_ErrorPortUnresponsiveDuringAllocation)
    DUMP_CASE (OMX_ErrorPortUnresponsiveDuringDeallocation)
    DUMP_CASE (OMX_ErrorPortUnresponsiveDuringStop)
    DUMP_CASE (OMX_ErrorIncorrectStateTransition)
    DUMP_CASE (OMX_ErrorIncorrectStateOperation)
    DUMP_CASE (OMX_ErrorUnsupportedSetting)
    DUMP_CASE (OMX_ErrorUnsupportedIndex)
    DUMP_CASE (OMX_ErrorBadPortIndex)
    DUMP_CASE (OMX_ErrorPortUnpopulated)
    DUMP_CASE (OMX_ErrorComponentSuspended)
    DUMP_CASE (OMX_ErrorDynamicResourcesUnavailable)
    DUMP_CASE (OMX_ErrorMbErrorsInFrame)
    DUMP_CASE (OMX_ErrorFormatNotDetected)
    DUMP_CASE (OMX_ErrorContentPipeOpenFailed)
    DUMP_CASE (OMX_ErrorContentPipeCreationFailed)
    DUMP_CASE (OMX_ErrorSeperateTablesUsed)
    DUMP_CASE (OMX_ErrorTunnelingUnsupported)
    DUMP_CASE (OMX_ErrorDiskFull)
    DUMP_CASE (OMX_ErrorMaxFileSize)
    DUMP_CASE (OMX_ErrorDrmUnauthorised)
    DUMP_CASE (OMX_ErrorDrmExpired)
    DUMP_CASE (OMX_ErrorDrmGeneral)
    default: return "unknown OMX_ERRORTYPE";
  }
}


int omxInit(omxState_t** handle,
            uint32_t frame_width, uint32_t frame_height, uint32_t num_video_buffers, uint32_t use_deinterlace,  // image_fx
            uint32_t x_offset, uint32_t y_offset, uint32_t width, uint32_t height, uint32_t disp_num,           // video_render
            uint32_t freq, uint32_t channels, uint32_t bits, uint32_t num_audio_buffers, uint32_t buffer_size)  // audio_render
{
    OMX_ERRORTYPE ret;
    omxState_t* omxState;
    *handle = NULL;

    if((omxState = (omxState_t*)malloc(sizeof(omxState_t))) == NULL)
    {
        return 1;
    }

    *handle = omxState;

    memset(omxState->list, 0, sizeof(omxState->list));
    memset(omxState->tunnel, 0, sizeof(omxState->tunnel));

    if((omxState->client = ilclient_init()) == NULL)
    {
        return 1;
    }

    if((ret = OMX_Init()) != OMX_ErrorNone)
    {
        ilclient_destroy(omxState->client);
        return 2;
    }

  // create image_fx
    if (setupImageFx(omxState, frame_width, frame_height, num_video_buffers, use_deinterlace) != 0)
    {
        fprintf(stderr, "%s() - Error: setupImageFx() failed\n", __FUNCTION__);
        return 3;
    }

    omxState->list[0] = omxState->image_fx;

  // create video_scheduler
    if((ret = ilclient_create_component(omxState->client, &omxState->video_scheduler, "video_scheduler", ILCLIENT_DISABLE_ALL_PORTS)) != OMX_ErrorNone)
    {
        fprintf(stderr, "%s() - Error: ilclient_create_component(video_scheduler) failed\n", __FUNCTION__);
        return 3;
    }

    omxState->list[1] = omxState->video_scheduler;

  // create video_render
    if (setupVideoRender(omxState, x_offset, y_offset, width, height, disp_num) != 0)
    {
        fprintf(stderr, "%s() - Error: setupVideoRender() failed\n", __FUNCTION__);
        return 3;
    }

    omxState->list[2] = omxState->video_render;

  // create clock
    if(setupClock(omxState) != 0)
    {
        fprintf(stderr, "%s() - Error: setupClock() failed\n", __FUNCTION__);
        return 3;
    }

    omxState->list[3] = omxState->clock;

  // create audio_render
    if(setupAudioRender(omxState, freq, channels, bits, num_audio_buffers, buffer_size) != 0)
    {
        fprintf(stderr, "%s() - Error: AudioRender() failed\n", __FUNCTION__);
        return 3;
    }

    omxState->list[4] = omxState->audio_render;

    set_tunnel(omxState->tunnel, omxState->image_fx, 191, omxState->video_scheduler, 10);
    set_tunnel(omxState->tunnel+1, omxState->video_scheduler, 11, omxState->video_render, 90);
    set_tunnel(omxState->tunnel+2, omxState->clock, 80, omxState->video_scheduler, 12);
    set_tunnel(omxState->tunnel+3, omxState->clock, 81, omxState->audio_render, 101);

  // setup clock->scheduler tunnel first
    if((ret = ilclient_setup_tunnel(omxState->tunnel+2, 0, 0)) != OMX_ErrorNone)
    {
        fprintf(stderr, "%s() - Error: ilclient_setup_tunnel(clock->scheduler) failed: %s\n", __FUNCTION__, dump_tunnel_err(ret));
        return 6;
    }

  // setup clock->audio_render tunnel
    if((ret = ilclient_setup_tunnel(omxState->tunnel+3, 0, 0)) != OMX_ErrorNone)
    {
        fprintf(stderr, "%s() - Error: ilclient_setup_tunnel(clock->audio_render) failed: %s\n", __FUNCTION__, dump_tunnel_err(ret));
        return 6;
    }

    ilclient_change_component_state(omxState->clock, OMX_StateExecuting);
    ilclient_change_component_state(omxState->audio_render, OMX_StateExecuting);

  // setup image_fx->video_scheduler tunnel
    if((ret = ilclient_setup_tunnel(omxState->tunnel, 0, 0)) != 0)
    {
        fprintf(stderr, "%s() - Error: ilclient_setup_tunnel(image_fx->video_scheduler) failed: %s\n",
                        __FUNCTION__, dump_tunnel_err(ret));
        return 6;
    }

    ilclient_change_component_state(omxState->video_scheduler, OMX_StateExecuting);

  // setup video_scheduler->video_render tunnel
    if((ret = ilclient_setup_tunnel(omxState->tunnel+1, 0, 1000)) != 0)
    {
        fprintf(stderr, "%s() - Error: ilclient_setup_tunnel(video_scheduler->video_render) failed: %s\n",
                        __FUNCTION__, dump_tunnel_err(ret));
        return 6;
    }

    ilclient_change_component_state(omxState->video_render, OMX_StateExecuting);
    ilclient_change_component_state(omxState->image_fx, OMX_StateExecuting);

    fprintf(stderr, "%s() - Info: OMX pipeline initialized\n", __FUNCTION__);

    return 0;
}


int omxDeinit(omxState_t* omxState)
{
#define fprintf_(args...)	((void)0)
//#define fprintf_ 		fprintf

    int ret;

    if(omxState->video_buf != NULL)
    {
        fprintf_(stderr, "1\n");

        omxState->video_buf->nFilledLen = 0;
        omxState->video_buf->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN | OMX_BUFFERFLAG_EOS;

        if((ret = OMX_EmptyThisBuffer(ILC_GET_HANDLE(omxState->image_fx), omxState->video_buf)) != OMX_ErrorNone)
        {
            fprintf(stderr, "%s() - Error: OMX_EmptyThisBuffer() failed: %s\n", __FUNCTION__, dump_OMX_ERRORTYPE(ret));
        }

        fprintf_(stderr, "2\n");

        // wait for EOS from render
        fprintf(stderr, "%s() - Info: waiting for video_render EOS event...\n", __FUNCTION__);
        ret = ilclient_wait_for_event(omxState->video_render, OMX_EventBufferFlag, 90, 0, OMX_BUFFERFLAG_EOS, 0, ILCLIENT_BUFFER_FLAG_EOS, -1);
        fprintf(stderr, "%s() - Info: done wait status: %s\n", __FUNCTION__,
                ret == -3 ? "config changed" : ret == -2 ? "failed" : ret == -1 ? "timeout" : "success");

    }
    // need to flush the renderer to allow image_fx to disable its input port
    ilclient_flush_tunnels(omxState->tunnel, 0);
    ilclient_disable_tunnel(omxState->tunnel);
    ilclient_disable_tunnel(omxState->tunnel+1);
    ilclient_disable_tunnel(omxState->tunnel+2);
    ilclient_disable_tunnel(omxState->tunnel+3);

    fprintf_(stderr, "6\n");

// image_fx - old deinit (freezes without sleep before omxDeinit)
    //ilclient_disable_port_buffers(omxState->image_fx, 190, NULL, NULL, NULL);
// image_fx - old deinit

    fprintf_(stderr, "7\n");
    ilclient_teardown_tunnels(omxState->tunnel);
    fprintf_(stderr, "8\n");

    ilclient_state_transition(omxState->list, OMX_StateIdle);
    fprintf_(stderr, "9\n");

// audio
    if((ret = OMX_SendCommand(ILC_GET_HANDLE(omxState->audio_render), OMX_CommandStateSet, OMX_StateLoaded, NULL)) != OMX_ErrorNone)
    {
        fprintf(stderr, "%s() - Error: OMX_SendCommand(audio_render, OMX_StateLoaded) failed: %s\n",
                        __FUNCTION__, dump_OMX_ERRORTYPE(ret));
    }
    fprintf_(stderr, "10\n");
    ilclient_disable_port_buffers(omxState->audio_render, 100, NULL, NULL, NULL);
    fprintf_(stderr, "11\n");
// audio

    usleep(50*1000);

// image_fx - new deinit
    if((ret = OMX_SendCommand(ILC_GET_HANDLE(omxState->image_fx), OMX_CommandStateSet, OMX_StateLoaded, NULL)) != OMX_ErrorNone)
    {
        fprintf(stderr, "%s() - Error: OMX_SendCommand(image_fx, OMX_StateLoaded) failed: %s\n", __FUNCTION__, dump_OMX_ERRORTYPE(ret));
    }
    fprintf_(stderr, "12\n");
    ilclient_disable_port_buffers(omxState->image_fx, 190, NULL, NULL, NULL);
    fprintf_(stderr, "13\n");
// image_fx - new deinit

    ilclient_state_transition(omxState->list, OMX_StateLoaded);
    fprintf_(stderr, "14\n");

    ilclient_cleanup_components(omxState->list);
    fprintf_(stderr, "15\n");

    if((ret = OMX_Deinit()) != OMX_ErrorNone)
    {
        fprintf(stderr, "%s() - Error: OMX_Deinit() failed: %s\n", __FUNCTION__, dump_OMX_ERRORTYPE(ret));
    }

    ilclient_destroy(omxState->client);

    fprintf(stderr, "%s() - Info: OMX pipeline deinitialized\n", __FUNCTION__);

    return 0;
}
