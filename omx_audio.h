#ifndef _OMX_AUDIO_
#define _OMX_AUDIO_

#include "omx_integration.h"

#define OUT_CHANNELS(num_channels) ((num_channels) > 4 ? 8: (num_channels) > 2 ? 4: (num_channels))

int setupAudioRender(omxState_t* omxState, uint32_t freq, uint32_t channels, uint32_t bits, uint32_t num_buffers, uint32_t buffer_size);
int audioGetFrame(omxState_t* omxState);
int audioPutFrame(omxState_t* omxState);
uint32_t audioGetLatency(omxState_t* omxState);

#endif
