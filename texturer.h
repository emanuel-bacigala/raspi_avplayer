//-------------------------------------------------------------------------
// Copyright (c) 2013 Matus Jurecka
//
//-------------------------------------------------------------------------


#include "bcm_host.h"

#  ifdef __cplusplus
extern "C" {
#  endif /* __cplusplus */

DISPMANX_MODEINFO_T* texturerInit(int disp_no, int opacity);
DISPMANX_DISPLAY_HANDLE_T* texturerGetDisplayHandle(void);

int texturerAddResource(uint32_t t_w, uint32_t t_h, uint32_t s_x, uint32_t s_y, uint32_t s_w, uint32_t s_h, uint32_t bpp);
inline int texturerChangeElement(int resId, uint32_t s_x, uint32_t s_y, uint32_t s_w, uint32_t s_h);
inline void texturerBeginDrawFrame(void);
inline void texturerEndDrawFrame(void);
inline void texturerEndDrawFrameNoSync(void);
void texturerSetData(unsigned char *image, int resId);
inline void texturerExchangeDisplayDoubleBuffer(int resId);
void texturerDestroy(void);

#  ifdef __cplusplus
}
#  endif /* __cplusplus */

