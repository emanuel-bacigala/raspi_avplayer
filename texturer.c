//-------------------------------------------------------------------------
// Copyright (c) 2013 Matus Jurecka
//
//-------------------------------------------------------------------------

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "bcm_host.h"


#define MAX_RESOURCES   8
#define RGB565(r,g,b) (((r)>>3)<<11 | ((g)>>2)<<5 | (b)>>3)
#define ALIGN_TO_16(x)  ((x + 15) & ~15)

#ifndef ALIGN_UP
#define ALIGN_UP(x,y)  ((x + (y)-1) & ~((y)-1))
#endif

// resource
static uint16_t pal[256];
uint32_t texWidth[MAX_RESOURCES], texHeight[MAX_RESOURCES], texBpp[MAX_RESOURCES], texBpl[MAX_RESOURCES];
int numResources;
VC_IMAGE_TYPE_T type[MAX_RESOURCES];
VC_RECT_T dst_rect[MAX_RESOURCES];
DISPMANX_RESOURCE_HANDLE_T resource[MAX_RESOURCES];
DISPMANX_RESOURCE_HANDLE_T resource_db[MAX_RESOURCES];
DISPMANX_ELEMENT_HANDLE_T element[MAX_RESOURCES];
DISPMANX_RESOURCE_HANDLE_T temp;
// resource

// display
DISPMANX_DISPLAY_HANDLE_T display;
DISPMANX_MODEINFO_T info;
DISPMANX_UPDATE_HANDLE_T update;
VC_DISPMANX_ALPHA_T alpha =
{
    DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS,
    255, //alpha 0->255
    0
};
// display

DISPMANX_MODEINFO_T* texturerInit(int disp_no, int opacity)
{
    int result,i;

// 8BPP grayscale palette init
    for(i=0;i<256;i++)
    {
        pal[i] = RGB565(i,i,i);
    }
// 8BPP grayscale palette init

    numResources = 0;
    //bcm_host_init();

    //assert(vc_dispman_init() != 0);

    display = vc_dispmanx_display_open(disp_no);
    assert(display != 0);

    result = vc_dispmanx_display_get_info(display, &info);
    assert(result == 0);

    alpha.opacity = opacity;

    return &info;
}


DISPMANX_DISPLAY_HANDLE_T* texturerGetDisplayHandle(void)
{
    return &display;
}

                      //width|pitch<<16 (pitch (pocet bajtov) musi byt align 32)
int texturerAddResource(uint32_t t_w, uint32_t t_h, uint32_t s_x, uint32_t s_y, uint32_t s_w, uint32_t s_h, uint32_t bpp)
{
    int result;
    uint32_t unused;
    VC_RECT_T src_rect;

    texWidth[numResources] = t_w;
    texHeight[numResources] = t_h;
    texBpp[numResources] = bpp;
    texBpl[numResources] = t_w >> 16;

// toto by malo byt koli korektnosti
    //assert(((t_w >> 16) & 0xFFFF) == (ALIGN_UP(t_w,32) & 0xFFFF)*(bpp/8)); // pitch == ALIGN_UP(width)*(bpp/8)
    //assert(((t_w >> 16) & 0xFFFF) % 32 == 0);                              // pitch % 32 == 0
    //assert(((t_h >> 16) & 0xFFFF) % 16 == 0);                              // aligned_height % 16 == 0
// toto by malo byt koli korektnosti

    //texBpl[numResources] = t_w*bpp/8;
    //texBpl[numResources] = (ALIGN_UP(t_w*bpp/8,32));
    //texAlignedHeight = ALIGN_TO_16(texHeight);
    //texSize = texBpl * texAlignedHeight;

    if (bpp == 32) //VC_IMAGE_XRGB8888 - OK pre (xvfb aj /dev/fb0) 32bit framebuffer; //VC_IMAGE_RGBX32 - OK pre raw 32bit
    {
        type[numResources] = VC_IMAGE_RGBX32; // VC_IMAGE_RGBX32 VC_IMAGE_BGRX8888 VC_IMAGE_RGBX8888 VC_IMAGE_ARGB8888 VC_IMAGE_XRGB8888
        resource[numResources] = vc_dispmanx_resource_create(type[numResources], texWidth[numResources], texHeight[numResources], &unused ); // ide aj toto tak aky problem ??
        assert(resource[numResources] != 0);
        resource_db[numResources] = vc_dispmanx_resource_create(type[numResources], texWidth[numResources], texHeight[numResources], &unused ); // ide aj toto tak aky problem ??
        assert(resource_db[numResources] != 0);
    }
    if (bpp == 24)
    {
        type[numResources] = VC_IMAGE_RGB888; //VC_IMAGE_RGB888 - OK pre 24bit framebuffer; VC_IMAGE_BGR888 - OK pre raw 24bit
        resource[numResources] = vc_dispmanx_resource_create(type[numResources], texWidth[numResources], texHeight[numResources], &unused ); // ide aj toto tak aky problem ??
        assert(resource[numResources] != 0);
        resource_db[numResources] = vc_dispmanx_resource_create(type[numResources], texWidth[numResources], texHeight[numResources], &unused ); // ide aj toto tak aky problem ??
        assert(resource_db[numResources] != 0);
    }
    if (bpp == 16)
    {
        type[numResources] = VC_IMAGE_RGB565;
        resource[numResources] = vc_dispmanx_resource_create(type[numResources], texWidth[numResources], texHeight[numResources], &unused ); // ide aj toto tak aky problem ??
        assert(resource[numResources] != 0);
        resource_db[numResources] = vc_dispmanx_resource_create(type[numResources], texWidth[numResources], texHeight[numResources], &unused ); // ide aj toto tak aky problem ??
        assert(resource_db[numResources] != 0);
    }
    else if (bpp == 12)
    {
        type[numResources] = VC_IMAGE_YUV420;
        resource[numResources] = vc_dispmanx_resource_create(type[numResources], texWidth[numResources], texHeight[numResources], &unused ); // ide aj toto t$
        assert(resource[numResources] != 0);
        resource_db[numResources] = vc_dispmanx_resource_create(type[numResources], texWidth[numResources], texHeight[numResources], &unused ); // ide aj tot$
        assert(resource_db[numResources] != 0);

    }
    else if (bpp == 8)
    {
/* pitch = width*bpp/8 aligned_up to 32 (height aligned to 16)
 poskusaj  width|(pitch*2)<<16 */

        type[numResources] = VC_IMAGE_8BPP;
        resource[numResources]  = vc_dispmanx_resource_create(type[numResources], texWidth[numResources], texHeight[numResources], &unused ); // ide aj toto tak aky problem ??
        assert(resource[numResources] != 0);
        resource_db[numResources] = vc_dispmanx_resource_create(type[numResources], texWidth[numResources], texHeight[numResources], &unused ); // ide aj toto tak aky problem ??
        assert(resource_db[numResources] != 0);
        result = vc_dispmanx_resource_set_palette( resource[numResources], pal, 0, sizeof pal );
        assert( result == 0 );
        result = vc_dispmanx_resource_set_palette( resource_db[numResources], pal, 0, sizeof pal );
        assert( result == 0 );
    }


    vc_dispmanx_rect_set(&src_rect, 0, 0, texWidth[numResources] << 16, texHeight[numResources] << 16);
    //vc_dispmanx_rect_set(&dst_rect, 0, 0, info.width, info.height);  //full screen
    vc_dispmanx_rect_set(&dst_rect[numResources], s_x & 0xFFFF, s_y & 0xFFFF, s_w & 0xFFFF, s_h & 0xFFFF);  //tex size

    update = vc_dispmanx_update_start(0);
    assert(update != 0);

    element[numResources] = vc_dispmanx_element_add(update,
                                display,
                                0, // layer
                                &dst_rect[numResources],
                                resource_db[numResources],
                                &src_rect,
                                DISPMANX_PROTECTION_NONE,
                                &alpha,
                                NULL, // clamp
                                DISPMANX_NO_ROTATE);  // pozri /opt/vc/include/interface/vmcs_host/vc_dispmanx_types.h
                                //DISPMANX_NO_ROTATE
                                //DISPMANX_ROTATE_180);
    assert(element[numResources] != 0);

    //---------------------------------------------------------------------

    result = vc_dispmanx_update_submit_sync(update);
    assert(result == 0);

    if (bpp==12)
    {
        vc_dispmanx_rect_set(&dst_rect[numResources], 0, 0,
//                             texWidth[numResources] & 0xFFFF, (3*ALIGN_UP(texHeight[numResources] & 0xFFFF, 16))/2);
                             texWidth[numResources] & 0xFFFF, (3*(texHeight[numResources] >> 16))/2 );
        //texBpl[numResources] = ALIGN_UP(t_w,32);
    }
//    else if (bpp==24)
//        vc_dispmanx_rect_set(&dst_rect[numResources], 0, 0, texBpl[numResources] & 0xFFFF, (3*(texHeight[numResources] >> 16)));
    else
    {
        // aby to nebolo v texturerSetData()
        vc_dispmanx_rect_set(&dst_rect[numResources], 0, 0, texBpl[numResources] & 0xFFFF, (texHeight[numResources] >> 16) & 0xFFFF);
    }

    fprintf(stderr, "%s() - Info: resource(%d) width=%u/%u height=%u/%u bpp=%d\n",
            __FUNCTION__, numResources, texWidth[numResources] & 0xFFFF, texBpl[numResources],
            texHeight[numResources]& 0xFFFF, texHeight[numResources]>> 16, texBpp[numResources]);

    return numResources++;
}


int texturerChangeElement(int resId, uint32_t s_x, uint32_t s_y, uint32_t s_w, uint32_t s_h)
{
    int result;
    VC_RECT_T dst_rect2;

    vc_dispmanx_rect_set(&dst_rect2, s_x, s_y, s_w, s_h);  //tex size
    update = vc_dispmanx_update_start(0);
    result = vc_dispmanx_element_change_attributes(update, element[resId], 0, 0, 0, &dst_rect2, 0, 0, DISPMANX_NO_ROTATE);
    vc_dispmanx_update_submit_sync(update);
    assert(result == 0);
    return result;
}


void texturerBeginDrawFrame(void)
{
    update = vc_dispmanx_update_start( 0 );
}


void texturerEndDrawFrame(void)
{
    vc_dispmanx_update_submit_sync(update);         // update with sync (60FPS)

}

void texturerEndDrawFrameNoSync(void)
{
    vc_dispmanx_update_submit( update, 0, 0 );    // update with no sync (can run cca 400FPS)

}

void texturerSetData(unsigned char *image, int resId)
{
    //fprintf(stderr, "BPL:%d\n",texBpl[resId]);
    vc_dispmanx_resource_write_data( resource[resId], type[resId], texBpl[resId], image, &dst_rect[resId] );
}


void texturerExchangeDisplayDoubleBuffer(int resId)
{
    vc_dispmanx_element_change_source( update, element[resId], resource[resId] );

    temp = resource[resId];
    resource[resId] = resource_db[resId];
    resource_db[resId] = temp;
}


void texturerDestroy(void)
{
    int result,i;

    update = vc_dispmanx_update_start(0);
    assert(update != 0);

    for(i=0;i<numResources;i++)
    {
        result = vc_dispmanx_element_remove(update, element[i]);
        assert(result == 0);
    }

    result = vc_dispmanx_update_submit_sync(update);
    assert(result == 0);

    for(i=0;i<numResources;i++)
    {
        result = vc_dispmanx_resource_delete(resource[i]);
        assert(result == 0);
        result = vc_dispmanx_resource_delete(resource_db[i]);
        assert(result == 0);
    }

    result = vc_dispmanx_display_close(display);
    assert(result == 0);

    //tuhol disp4
    //vc_dispmanx_stop();
}
