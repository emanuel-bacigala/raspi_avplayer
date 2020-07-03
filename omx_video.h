#ifndef _OMX_VIDEO_
#define _OMX_VIDEO_

#include "omx_integration.h"


int setupVideoRender(omxState_t* omxState, uint32_t x_offset, uint32_t y_offset, uint32_t width, uint32_t height, uint32_t disp_num);
int setupImageFx(omxState_t* omxState, uint32_t frame_width, uint32_t frame_height, uint32_t num_buffers, uint32_t use_filter);
int videoSetDeinterlace(omxState_t* omxState, int type);
int videoGetFrame(omxState_t* omxState);
int videoPutFrame(omxState_t* omxState);

#endif
