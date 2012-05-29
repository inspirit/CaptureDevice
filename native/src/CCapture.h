#ifndef CCAPTURE_H_
#define CCAPTURE_H_

#include "CCaptureConfig.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct CaptureDeviceInfo {
        char name[255];
        int name_size;
        int available, connected;
    } CaptureDeviceInfo;

    typedef struct CCapture CCapture;

    CAPI(CCapture*) createCameraCapture (int width, int height, char *deviceName, int frameRate );
    CAPI(void) releaseCapture( CCapture* capture );
    CAPI(int) getCaptureDevices( CaptureDeviceInfo* result, int forceRefresh );
    CAPI(void) captureStart( CCapture* capture );
    CAPI(void) captureStop( CCapture* capture );
    CAPI(void) captureFocusAtPoint(CCapture *capture, float x, float y);
    CAPI(void) captureGetStillImage(CCapture *capture, void (*callback)(const uint8_t *data, int32_t size));
    CAPI(int) captureCheckResponse( CCapture* capture );
    CAPI(int) captureCheckNewFrame( CCapture* capture );
    CAPI(int) captureIsCapturing( CCapture* capture );
    CAPI(const unsigned char*) captureGetFrame( CCapture* capture, int*, int*, int* );
    CAPI(void) captureGetSize( CCapture* capture, int* width, int* height );
    CAPI(int) captureSupportsSaveToCameraRoll();
    CAPI(int) captureSaveToCameraRoll( const char *fileName, const uint8_t* data, int size );

#ifdef __cplusplus
}
#endif

#endif