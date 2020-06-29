#include <stdio.h>
#include "bcm_host.h"
#include "ilclient.h"
#include "omx_audio.h"


int setupAudioRender(omxState_t* omxState, uint32_t freq, uint32_t channels, uint32_t bits, uint32_t num_buffers, uint32_t buffer_size)
{
    OMX_ERRORTYPE error;
    uint32_t bytes_per_sample = (bits * OUT_CHANNELS(channels)) >> 3;

  // basic sanity check on arguments
    if(!(freq >= 8000 && freq <= 192000 &&
       (channels >= 1 && channels <= 8) &&
       (bits == 16 || bits == 32) &&
       num_buffers > 0 && buffer_size >= bytes_per_sample))
    {
        fprintf(stderr, "%s() - Error: input arguments mismatch\n", __FUNCTION__);
        return 1;
    }

    if(ilclient_create_component(omxState->client, &omxState->audio_render, "audio_render", ILCLIENT_DISABLE_ALL_PORTS | ILCLIENT_ENABLE_INPUT_BUFFERS) != OMX_ErrorNone)
    {
        fprintf(stderr, "%s() - Error: ilclient_create_component(audio_render) failed\n", __FUNCTION__);
        return 3;
    }

  // set up the number/size of buffers
    OMX_PARAM_PORTDEFINITIONTYPE param;
    OMX_INIT_STRUCTURE(param);
    param.nPortIndex = 100;

    if ((error = OMX_GetParameter(ILC_GET_HANDLE(omxState->audio_render), OMX_IndexParamPortDefinition, &param)) != OMX_ErrorNone)
    {
        fprintf(stderr, "%s() - Error: OMX_GetParameter(audio_render, OMX_IndexParamPortDefinition) failed\n", __FUNCTION__);
        return 2;
    }

    param.nBufferSize = (buffer_size + 15) & ~15;  // buffer lengths must be 16 byte aligned for VCHI
    param.nBufferCountActual = num_buffers;

    if ((error = OMX_SetParameter(ILC_GET_HANDLE(omxState->audio_render), OMX_IndexParamPortDefinition, &param)) != OMX_ErrorNone)
    {
        fprintf(stderr, "%s() - Error: OMX_SetParameter(audio_render, OMX_IndexParamPortDefinition) failed\n", __FUNCTION__);
        return 3;
    }

    fprintf(stderr, "%s() - Info: input buffer count=%d\n", __FUNCTION__, param.nBufferCountActual);

  // set the pcm parameters
    OMX_AUDIO_PARAM_PCMMODETYPE pcm;
    OMX_INIT_STRUCTURE(pcm);
    pcm.nPortIndex = 100;
    pcm.nChannels = OUT_CHANNELS(channels);
    pcm.eNumData = OMX_NumericalDataSigned;
    pcm.eEndian = OMX_EndianLittle;
    pcm.nSamplingRate = freq;
    pcm.bInterleaved = OMX_TRUE;
    pcm.nBitPerSample = bits;
    pcm.ePCMMode = OMX_AUDIO_PCMModeLinear;

    switch(channels)
    {
        case 1:
            pcm.eChannelMapping[0] = OMX_AUDIO_ChannelCF;
            break;
        case 3:
            pcm.eChannelMapping[2] = OMX_AUDIO_ChannelCF;
            pcm.eChannelMapping[1] = OMX_AUDIO_ChannelRF;
            pcm.eChannelMapping[0] = OMX_AUDIO_ChannelLF;
            break;
        case 8:
            pcm.eChannelMapping[7] = OMX_AUDIO_ChannelRS;
        case 7:
            pcm.eChannelMapping[6] = OMX_AUDIO_ChannelLS;
        case 6:
            pcm.eChannelMapping[5] = OMX_AUDIO_ChannelRR;
        case 5:
            pcm.eChannelMapping[4] = OMX_AUDIO_ChannelLR;
        case 4:
            pcm.eChannelMapping[3] = OMX_AUDIO_ChannelLFE;
            pcm.eChannelMapping[2] = OMX_AUDIO_ChannelCF;
        case 2:
            pcm.eChannelMapping[1] = OMX_AUDIO_ChannelRF;
            pcm.eChannelMapping[0] = OMX_AUDIO_ChannelLF;
            break;
    }

    if ((error = OMX_SetParameter(ILC_GET_HANDLE(omxState->audio_render), OMX_IndexParamAudioPcm, &pcm)) != OMX_ErrorNone)
    {
        fprintf(stderr, "%s() - Error: OMX_SetParameter(audio_render, OMX_IndexParamAudioPcm) failed\n", __FUNCTION__);
        return 3;
    }

  // change state to idle
    ilclient_change_component_state(omxState->audio_render, OMX_StateIdle);

  // enable input port bufferes
    if (ilclient_enable_port_buffers(omxState->audio_render, 100, NULL, NULL, NULL) < 0)
    {
        fprintf(stderr, "%s() - Error: ilclient_enable_port_buffers(audio_render, 100) failed\n", __FUNCTION__);
        ilclient_change_component_state(omxState->audio_render, OMX_StateLoaded);

        COMPONENT_T *list[2];
        list[0] = omxState->audio_render;
        list[1] = 0;
        ilclient_cleanup_components(list);

        return 4;
    }

  // set destination
    OMX_CONFIG_BRCMAUDIODESTINATIONTYPE ar_dest;
    OMX_INIT_STRUCTURE(ar_dest);
    strcpy((char *)ar_dest.sName, "local");
    if (OMX_SetConfig(ILC_GET_HANDLE(omxState->audio_render), OMX_IndexConfigBrcmAudioDestination, &ar_dest) != OMX_ErrorNone)
    {
        fprintf(stderr, "%s() - Error: OMX_SetConfig(audio_render, OMX_IndexConfigBrcmAudioDestination)\n", __FUNCTION__);
        return 7;
    }
/*
  // disable clock master
    OMX_CONFIG_BOOLEANTYPE ismaster;
    OMX_INIT_STRUCTURE(ismaster);
    ismaster.bEnabled = OMX_FALSE;
    if(OMX_SetConfig(ILC_GET_HANDLE(omxState->audio_render), OMX_IndexConfigBrcmClockReferenceSource, &ismaster) != OMX_ErrorNone)
    {
        fprintf(stderr, "%s() - Error: OMX_SetConfig(audio_render, OMX_IndexConfigBrcmClockReferenceSource)\n", __FUNCTION__);
        return 4;
    }
*/
/*
  // set volume - nejde
    OMX_AUDIO_CONFIG_VOLUMETYPE volume;
    OMX_INIT_STRUCTURE(volume);
    volume.bLinear = OMX_TRUE;
    volume.sVolume.nValue = 99;  // 0..100
    volume.sVolume.nMin = 98;  // 0..100
    volume.sVolume.nMax = 100;  // 0..100

    if (OMX_SetConfig(ILC_GET_HANDLE(omxState->audio_render), OMX_IndexConfigAudioVolume, &volume) != OMX_ErrorNone)
    {
        fprintf(stderr, "%s() - Error: OMX_SetConfig(audio_render, OMX_IndexConfigAudioVolume)\n", __FUNCTION__);
        return 7;
    }
*/
/*
  // set mute - nejde
    OMX_AUDIO_CONFIG_MUTETYPE mute;
    OMX_INIT_STRUCTURE(mute);
    mute.bMute = OMX_TRUE;

    if (OMX_SetConfig(ILC_GET_HANDLE(omxState->audio_render), OMX_IndexConfigAudioMute, &mute) != OMX_ErrorNone)
    {
        fprintf(stderr, "%s() - Error: OMX_SetConfig(audio_render, OMX_IndexConfigAudioMute)\n", __FUNCTION__);
        return 7;
    }
*/
    return 0;
}


int audioGetFrame(omxState_t* omxState)
{
    while((omxState->audio_buf = ilclient_get_input_buffer(omxState->audio_render, 100, 0)) == NULL)
        usleep(1000*10);

/*
    if((omxState->audio_buf = ilclient_get_input_buffer(omxState->audio_render, 100, 0)) == NULL)
    {
        fprintf(stderr, "%s() - Error: ilclient_get_input_buffer() failed\n", __FUNCTION__);
        return 1;
    }
*/
    return 0;
}


uint32_t audioGetLatency(omxState_t* omxState)
{
    OMX_PARAM_U32TYPE param;
    OMX_INIT_STRUCTURE(param);
    param.nPortIndex = 100;

    if (OMX_GetConfig(ILC_GET_HANDLE(omxState->audio_render), OMX_IndexConfigAudioRenderingLatency, &param) != OMX_ErrorNone)
    {
        fprintf(stderr, "%s() - Error: OMX_GetConfig(audio_render, OMX_IndexConfigAudioRenderingLatency)\n", __FUNCTION__);
        return -1; // uint32_t max. value ??
    }

    return param.nU32;
}


int audioPutFrame(omxState_t* omxState)
{
    //omxState->audio_buf->pAppPrivate = NULL;
    //omxState->audio_buf->nOffset = 0;

    if(OMX_EmptyThisBuffer(ILC_GET_HANDLE(omxState->audio_render), omxState->audio_buf) != OMX_ErrorNone)
    {
        fprintf(stderr, "%s() - Error: OMX_EmptyThisBuffer() failed\n", __FUNCTION__);
        return 1;
    }

    return 0;
}
