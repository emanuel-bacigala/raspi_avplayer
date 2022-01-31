#ifndef _OMX_INTEGRATION_
#define _OMX_INTEGRATION_

#include "bcm_host.h"
#include "ilclient.h"


#define OMX_INIT_STRUCTURE(a) \
memset(&(a), 0, sizeof(a)); \
(a).nSize = sizeof(a); \
(a).nVersion.s.nVersionMajor = OMX_VERSION_MAJOR; \
(a).nVersion.s.nVersionMinor = OMX_VERSION_MINOR; \
(a).nVersion.s.nRevision = OMX_VERSION_REVISION; \
(a).nVersion.s.nStep = OMX_VERSION_STEP


typedef struct omxState_t
{
    ILCLIENT_T *client;
    COMPONENT_T *image_fx;
    COMPONENT_T *video_scheduler;
    COMPONENT_T *video_render;
    COMPONENT_T *audio_render;
    COMPONENT_T *clock;
    COMPONENT_T *list[5+1];  // musi byt zakoncene nulou (+1), aby ilclient_flush_tunnels(),ilclient_teardown_tunnels(),
    TUNNEL_T tunnel[4+1];    // ilclient_s
    OMX_BUFFERHEADERTYPE *video_buf;
    OMX_BUFFERHEADERTYPE *audio_buf;
} omxState_t;


int omxInit(omxState_t** handle,
            uint32_t frame_width, uint32_t frame_height, uint32_t num_video_buffers, uint32_t use_deinterlace,  // image_fx
            uint32_t x_offset, uint32_t y_offset, uint32_t width, uint32_t height, uint32_t disp_num,           // video_render
            uint32_t freq, uint32_t channels, uint32_t bits, uint32_t num_audio_buffers, uint32_t buffer_size); // audio_render
int omxDeinit(omxState_t** _omxState);

#endif
