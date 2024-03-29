#include <stdio.h>
#include "bcm_host.h"
#include "ilclient.h"
#include "omx_video.h"
#include "dump.h"
#include "appdata.h"


uint32_t filterList[] = {
    OMX_ImageFilterNone,
    OMX_ImageFilterDeInterlaceLineDouble,
    OMX_ImageFilterDeInterlaceAdvanced,
    OMX_ImageFilterDeInterlaceFast,
    OMX_ImageFilterNegative,
    OMX_ImageFilterOilPaint,
    OMX_ImageFilterPastel,
    OMX_ImageFilterPosterise,
    OMX_ImageFilterCartoon,

    OMX_ImageFilterBlur,
    OMX_ImageFilterSketch,
    OMX_ImageFilterWashedOut,
    OMX_ImageFilterSharpen,

    OMX_ImageFilterNoise,
    OMX_ImageFilterEmboss,
    OMX_ImageFilterHatch,
    OMX_ImageFilterGpen,
    OMX_ImageFilterAntialias,
    OMX_ImageFilterDeRing,
    OMX_ImageFilterSolarize,
    OMX_ImageFilterWatercolor,
    OMX_ImageFilterFilm,
    OMX_ImageFilterSaturation,
    OMX_ImageFilterColourSwap,
    OMX_ImageFilterColourPoint,
    OMX_ImageFilterColourBalance,
    OMX_ImageFilterAnaglyph
};


int displayGetResolution(omxState_t** handle)
{
    DISPMANX_DISPLAY_HANDLE_T display;
    DISPMANX_MODEINFO_T info;
    int result;
    appData* userData = container_of(handle, appData, omxState);


    if ((display = vc_dispmanx_display_open(userData->displayNo)) == 0)
    {
        fprintf(stderr, "%s() - Error: vc_dispmanx_display_open() failed\n", __FUNCTION__);
        return 1;

    }

    if ((result = vc_dispmanx_display_get_info(display, &info)) != 0)
    {
        fprintf(stderr, "%s() - Error: vc_dispmanx_display_get_info() failed\n", __FUNCTION__);
        return 1;

    }

    if ((result = vc_dispmanx_display_close(display)) != 0)
    {
        fprintf(stderr, "%s() - Error: vc_dispmanx_display_close() failed\n", __FUNCTION__);
        return 1;

    }

    vc_dispmanx_stop();

    userData->screenWidth = info.width;
    userData->screenHeight = info.height;
    userData->renderWindow[2] = info.width;
    userData->renderWindow[3] = info.height;

    return 0;
}


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


int videoSetDislpayRegion(omxState_t* omxState, int x_offset, int y_offset, int width, int height)
{

    OMX_CONFIG_DISPLAYREGIONTYPE configDisplay;
    OMX_INIT_STRUCTURE(configDisplay);
    configDisplay.nPortIndex = 90;

  // set no aspect
    configDisplay.noaspect   = OMX_TRUE;
    configDisplay.set = (OMX_DISPLAYSETTYPE)(configDisplay.set|OMX_DISPLAY_SET_NOASPECT);

  // set display region
    configDisplay.set = (OMX_DISPLAYSETTYPE)(configDisplay.set|OMX_DISPLAY_SET_DEST_RECT|OMX_DISPLAY_SET_FULLSCREEN);
    configDisplay.dest_rect.x_offset = x_offset;
    configDisplay.dest_rect.y_offset = y_offset;
    configDisplay.dest_rect.width = width;
    configDisplay.dest_rect.height = height;

    if (OMX_SetConfig(ILC_GET_HANDLE(omxState->video_render), OMX_IndexConfigDisplayRegion, &configDisplay) != OMX_ErrorNone)
    {
        fprintf(stderr, "%s() - Error: OMX_SetConfig(video_render, OMX_IndexConfigDisplayRegion)\n", __FUNCTION__);
        return 1;
    }

    return 0;
}



int setupImageFx(omxState_t* omxState, uint32_t frame_width, uint32_t frame_height, uint32_t num_buffers, uint32_t useFilter)
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
    portdef.nBufferSize = portdef.format.image.nStride * portdef.format.image.nSliceHeight * 3 / 2;  // mandatory for old raspi FW
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

    if (filterList[useFilter] == OMX_ImageFilterDeInterlaceFast) // needed for anaglyph a deinterlaceFast
    {
        OMX_PARAM_U32TYPE extra_buffers;
        OMX_INIT_STRUCTURE(extra_buffers);
        extra_buffers.nU32 = -2;

        if (OMX_SetParameter(ILC_GET_HANDLE(omxState->image_fx), OMX_IndexParamBrcmExtraBuffers, &extra_buffers) != OMX_ErrorNone)
        {
            fprintf(stderr, "%s() - Error: OMX_SetParameter(image_fx, OMX_IndexParamBrcmExtraBuffers,)\n", __FUNCTION__);
            return 4;
        }
    }
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

    image_format.nNumParams = 4;
    image_format.nParams[0] = 3;
    image_format.nParams[1] = 0;  // default frame interval
    image_format.nParams[2] = 0;  // half framerate
    image_format.nParams[3] = 1;  // use qpus

    image_format.eImageFilter = filterList[useFilter];

    if (OMX_SetConfig(ILC_GET_HANDLE(omxState->image_fx), OMX_IndexConfigCommonImageFilterParameters, &image_format) != OMX_ErrorNone)
    {
        fprintf(stderr, "%s() - Error: OMX_SetParameter(image_fx, OMX_IndexConfigCommonImageFilterParameters)\n", __FUNCTION__);
        return 7;
    }

    fprintf(stderr, "%s() - Info: using image filter[%d]: %s\n", __FUNCTION__, useFilter, dump_OMX_IMAGEFILTERTYPE(image_format.eImageFilter));

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
    image_format.eImageFilter = filterList[type];

    if (OMX_SetConfig(ILC_GET_HANDLE(omxState->image_fx), OMX_IndexConfigCommonImageFilterParameters, &image_format) != OMX_ErrorNone)
    {
        fprintf(stderr, "%s() - Error: OMX_SetConfig(image_fx, OMX_IndexConfigCommonImageFilterParameters) failed\n", __FUNCTION__);
        return 1;
    }

    fprintf(stderr, "%s() - Info: using image filter[%d]: %s\n", __FUNCTION__, type, dump_OMX_IMAGEFILTERTYPE(image_format.eImageFilter));

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

