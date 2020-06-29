#include <stdio.h>
#include "bcm_host.h"
#include "ilclient.h"
#include "omx_video.h"


int setupVideoRender(omxState_t* omxState, uint32_t x_offset, uint32_t y_offset, uint32_t width, uint32_t height, uint32_t disp_num)
{
  // create video_render
    if(ilclient_create_component(omxState->client, &omxState->video_render, "video_render", ILCLIENT_DISABLE_ALL_PORTS) != OMX_ErrorNone)
    {
        fprintf(stderr, "%s() - Error: ilclient_create_component(video_render) failed\n", __FUNCTION__);
        return 3;
    }

    OMX_CONFIG_DISPLAYREGIONTYPE configDisplay;
    OMX_INIT_STRUCTURE(configDisplay);
    configDisplay.nPortIndex = 90;

  // set rotation + alpha + layer + dsp_number
    configDisplay.set = (OMX_DISPLAYSETTYPE)(OMX_DISPLAY_SET_TRANSFORM|OMX_DISPLAY_SET_ALPHA|OMX_DISPLAY_SET_LAYER|OMX_DISPLAY_SET_NUM);
    configDisplay.alpha = 255;
    configDisplay.num = disp_num;
    configDisplay.layer = (1<<15)-1;
    configDisplay.transform = 0;

  // set no aspect
    configDisplay.noaspect   = OMX_TRUE;
    configDisplay.set = (OMX_DISPLAYSETTYPE)(configDisplay.set|OMX_DISPLAY_SET_NOASPECT);
/*
  // set display region
    configDisplay.set = (OMX_DISPLAYSETTYPE)(configDisplay.set|OMX_DISPLAY_SET_DEST_RECT|OMX_DISPLAY_SET_FULLSCREEN);
    configDisplay.dest_rect.x_offset = x_offset;
    configDisplay.dest_rect.y_offset = y_offset;
    configDisplay.dest_rect.width = width;
    configDisplay.dest_rect.height = height;
*/
    if (OMX_SetConfig(ILC_GET_HANDLE(omxState->video_render), OMX_IndexConfigDisplayRegion, &configDisplay) != OMX_ErrorNone)
    {
        fprintf(stderr, "%s() - Error: OMX_SetConfig(video_render, OMX_IndexConfigDisplayRegion)\n", __FUNCTION__);
        return 7;
    }

    fprintf(stderr, "%s() - Info: using display(%d)\n", __FUNCTION__, disp_num);

    return 0;
}


int setupImageFx(omxState_t* omxState, uint32_t frame_width, uint32_t frame_height, uint32_t num_buffers, uint32_t use_deinterlace)
{
  // create image_fx
    if(ilclient_create_component(omxState->client, &omxState->image_fx, "image_fx", ILCLIENT_DISABLE_ALL_PORTS | ILCLIENT_ENABLE_INPUT_BUFFERS) != OMX_ErrorNone)
    {
        fprintf(stderr, "%s() - Error: ilclient_create_component(image_fx) failed\n", __FUNCTION__);
        return 3;
    }

  // need to setup the input for the image_fx
    OMX_PARAM_PORTDEFINITIONTYPE portdef;
    OMX_INIT_STRUCTURE(portdef);
    portdef.nPortIndex = 190;

    if (OMX_GetParameter(ILC_GET_HANDLE(omxState->image_fx), OMX_IndexParamPortDefinition, &portdef) != OMX_ErrorNone)
    {
        fprintf(stderr, "%s() - Error: OMX_GetParameter(image_fx, OMX_IndexParamPortDefinition)\n", __FUNCTION__);
        return 5;
    }

    portdef.nBufferCountActual = num_buffers;  // set input buffer count (max. 153 original 1)
    portdef.format.image.nFrameWidth = frame_width;
    portdef.format.image.nFrameHeight = frame_height;
    portdef.format.image.nStride = ALIGN_UP(portdef.format.image.nFrameWidth, 32);
    portdef.format.image.nSliceHeight = ALIGN_UP(portdef.format.image.nFrameHeight, 16);;
    //portdef.nBufferSize = portdef.format.image.nStride * portdef.format.image.nSliceHeight * 3 / 2;
    //portdef.format.image.eCompressionFormat = OMX_VIDEO_CodingUnused; // OMX_VIDEO_CodingUnused OMX_IMAGE_CodingUnused
    portdef.format.image.eColorFormat = OMX_COLOR_FormatYUV420PackedPlanar;

    if (OMX_SetParameter(ILC_GET_HANDLE(omxState->image_fx), OMX_IndexParamPortDefinition, &portdef) != OMX_ErrorNone)
    {
        fprintf(stderr, "%s() - Error: OMX_SetParameter(image_fx, OMX_IndexParamPortDefinition)\n", __FUNCTION__);
        return 4;
    }

    if (OMX_GetParameter(ILC_GET_HANDLE(omxState->image_fx), OMX_IndexParamPortDefinition, &portdef) != OMX_ErrorNone)
    {
        fprintf(stderr, "%s() - Error: OMX_GetParameter(image_fx, OMX_IndexParamPortDefinition)\n", __FUNCTION__);
        return 5;
    }

    fprintf(stderr, "%s() - Info: input buffer count=%d\n", __FUNCTION__, portdef.nBufferCountActual);


    ilclient_change_component_state(omxState->image_fx, OMX_StateIdle);

if(use_deinterlace)
{
/*
    // potrebuje len anaglyph a deinterlaceFast
    OMX_PARAM_U32TYPE extra_buffers;
    OMX_INIT_STRUCTURE(extra_buffers);
    extra_buffers.nU32 = -2;

    if (OMX_SetParameter(ILC_GET_HANDLE(omxState->image_fx), OMX_IndexParamBrcmExtraBuffers, &extra_buffers) != OMX_ErrorNone)
    {
        fprintf(stderr, "%s() - Error: OMX_SetParameter(image_fx, OMX_IndexParamBrcmExtraBuffers,)\n", __FUNCTION__);
        return 4;
    }
*/
// DEINTERLACE zapinaj LEN PRI STARTUP - inak to MOZE zatuhnut pri DEINIT
/*
  // bez parametrov
    OMX_CONFIG_IMAGEFILTERTYPE image_format;
    OMX_INIT_STRUCTURE(image_format);
    image_format.nPortIndex = 191;
    image_format.eImageFilter = OMX_ImageFilterNone; // OMX_ImageFilterDeInterlaceAdvanced
                                                                        // OMX_ImageFilterDeInterlaceLineDouble
                                                                        // OMX_ImageFilterDeInterlaceFast
                                                                        // OMX_ImageFilterNone
                                                                        // OMX_ImageFilterNegative
                                                                        // OMX_ImageFilterPastel
                                                                        // OMX_ImageFilterCartoon
                                                                        // OMX_ImageFilterEmboss

    if (OMX_SetConfig(ILC_GET_HANDLE(image_fx), OMX_IndexConfigCommonImageFilter, &image_format) != OMX_ErrorNone)
    {
        fprintf(stderr, "Error setting image_fx OMX_IndexConfigCommonImageFilter\n");
        exit(1);
    }
*/
  // s parametrami
    OMX_CONFIG_IMAGEFILTERPARAMSTYPE image_format;
    OMX_INIT_STRUCTURE(image_format);
    image_format.nPortIndex = 191;

#if 1
    image_format.nNumParams = 1;
    image_format.nParams[0] = 3;  // bolo 3 pre deint
    image_format.eImageFilter = OMX_ImageFilterDeInterlaceLineDouble;  // !!! DEINTERLACE MUSIM ZAPNUT TU ABY TO NETUHLO PRI DEINIT !!!
    //image_format.eImageFilter = OMX_ImageFilterDeInterlaceAdvanced;
    //image_format.eImageFilter = OMX_ImageFilterNone;
    //image_format.eImageFilter = OMX_ImageFilterPosterise;
    //image_format.eImageFilter = OMX_ImageFilterColourSwap;
    //image_format.eImageFilter = OMX_ImageFilterNegative;
#endif

#if 0
    image_format.nNumParams = 1;
    image_format.nParams[0] = 1;  // anaglyph z omxplayer
    image_format.eImageFilter = OMX_ImageFilterAnaglyph;
#endif

#if 0
    image_format.nNumParams = 4;
    image_format.nParams[0] = 3;
    image_format.nParams[1] = 0;  // default frame interval
    image_format.nParams[2] = 0;  // half framerate
    image_format.nParams[3] = 1;  // use qpus
    image_format.eImageFilter = OMX_ImageFilterDeInterlaceLineDouble;
    //image_format.eImageFilter = OMX_ImageFilterDeInterlaceAdvanced;
    //image_format.eImageFilter = OMX_ImageFilterDeInterlaceFast;
#endif

    if (OMX_SetConfig(ILC_GET_HANDLE(omxState->image_fx), OMX_IndexConfigCommonImageFilterParameters, &image_format) != OMX_ErrorNone)
    {
        fprintf(stderr, "%s() - Error: OMX_SetParameter(image_fx, OMX_IndexConfigCommonImageFilterParameters)\n", __FUNCTION__);
        return 7;
    }
// DEINTERLACE zapinaj LEN PRI STARTUP - inak to MOZE zatuhnut pri DEINIT
}

    if(ilclient_enable_port_buffers(omxState->image_fx, 190, NULL, NULL, NULL) != OMX_ErrorNone)
    {
        fprintf(stderr, "%s() - Error: ilclient_enable_port_buffers(image_fx, 190) failed\n", __FUNCTION__);
        ilclient_change_component_state(omxState->image_fx, OMX_StateLoaded);

        COMPONENT_T *list[2];
        list[0] = omxState->image_fx;
        list[1] = 0;
        ilclient_cleanup_components(list);

        return 9;
    }

    return 0;
}


int videoSetDeinterlace(omxState_t* omxState, int type)
{
  // s parametrami
    OMX_CONFIG_IMAGEFILTERPARAMSTYPE image_format;
    OMX_INIT_STRUCTURE(image_format);
    image_format.nPortIndex = 191;

    image_format.nNumParams = 4;
    image_format.nParams[0] = 3;
    image_format.nParams[1] = 0;
    image_format.nParams[2] = 0;
    image_format.nParams[3] = 1;
    image_format.eImageFilter = type ? /*OMX_ImageFilterNegative*/ OMX_ImageFilterDeInterlaceLineDouble /*OMX_ImageFilterDeInterlaceAdvanced*/ : OMX_ImageFilterNone;
    //image_format.eImageFilter = OMX_ImageFilterNone;

    //ilclient_change_component_state(omxState->image_fx, OMX_StateLoaded);

    if (OMX_SetConfig(ILC_GET_HANDLE(omxState->image_fx), OMX_IndexConfigCommonImageFilterParameters, &image_format) != OMX_ErrorNone)
    {
        fprintf(stderr, "%s() - Error: OMX_SetConfig(image_fx, OMX_IndexConfigCommonImageFilterParameters) failed\n", __FUNCTION__);
        return 1;
    }

    fprintf(stderr, "%s() - Info: deinterlation %s\n", __FUNCTION__, type ? "enabled" : "disabled");
    //ilclient_change_component_state(omxState->image_fx, OMX_StateExecuting);

    return 0;
}


int videoGetFrame(omxState_t* omxState)
{
    if((omxState->video_buf = ilclient_get_input_buffer(omxState->image_fx, 190, 1)) == NULL)
    {
        fprintf(stderr, "%s() - Error: ilclient_get_input_buffer() failed\n", __FUNCTION__);
        return 1;
    }

    return 0;
}


int videoPutFrame(omxState_t* omxState)
{
    if(OMX_EmptyThisBuffer(ILC_GET_HANDLE(omxState->image_fx), omxState->video_buf) != OMX_ErrorNone)
    {
        fprintf(stderr, "%s() - Error: OMX_EmptyThisBuffer() failed\n", __FUNCTION__);
        return 1;
    }

    return 0;
}

