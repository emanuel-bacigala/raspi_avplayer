#include "dump.h"

#define DUMP_CASE(x) case x: return #x;


const char* dump_OMX_IMAGEFILTERTYPE (OMX_IMAGEFILTERTYPE error){
  switch (error){
    DUMP_CASE (OMX_ImageFilterNone)
    DUMP_CASE (OMX_ImageFilterDeInterlaceLineDouble)
    DUMP_CASE (OMX_ImageFilterDeInterlaceAdvanced)
    DUMP_CASE (OMX_ImageFilterDeInterlaceFast)
    DUMP_CASE (OMX_ImageFilterNegative)
    DUMP_CASE (OMX_ImageFilterOilPaint)
    DUMP_CASE (OMX_ImageFilterPastel)
    DUMP_CASE (OMX_ImageFilterPosterise)
    DUMP_CASE (OMX_ImageFilterCartoon)
    DUMP_CASE (OMX_ImageFilterBlur)
    DUMP_CASE (OMX_ImageFilterSketch)
    DUMP_CASE (OMX_ImageFilterWashedOut)
    DUMP_CASE (OMX_ImageFilterSharpen)
    default: return "unknown OMX_IMAGEFILTERTYPE";
  }
}


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
