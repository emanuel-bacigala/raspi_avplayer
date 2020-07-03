#ifndef _OMX_DUMP_
#define _OMX_DUMP_

#include "bcm_host.h"
#include "ilclient.h"

#define DUMP_CASE(x) case x: return #x;

const char* dump_OMX_IMAGEFILTERTYPE (OMX_IMAGEFILTERTYPE error);
const char* dump_tunnel_err(int error);
const char* dump_OMX_ERRORTYPE (OMX_ERRORTYPE error);

#endif
