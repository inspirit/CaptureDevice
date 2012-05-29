
#include "Capture.h"
#include "CCapture.h"

#include <string.h>

#if defined (__ANDROID__)
#include <android/log.h>
#if !defined(LOGD) && !defined(LOGI) && !defined(LOGE)
#define LOG_TAG "CAPTURE"
#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))
#endif
#endif

using namespace std;

//
inline Capture& c_to_cpp(CCapture* ptr)
{
  return *reinterpret_cast<Capture*>(ptr);
}

inline const Capture& c_to_cpp(const CCapture* ptr)
{
  return *reinterpret_cast<const Capture*>(ptr);
}

inline CCapture* cpp_to_c(Capture& ref)
{
  return reinterpret_cast<CCapture*>(&ref);
}

inline const CCapture* cpp_to_c(const Capture& ref)
{
  return reinterpret_cast<const CCapture*>(&ref);
}
//

//
// C Interface
//

C_IMPL CCapture * createCameraCapture (int width, int height, char *deviceName, int frameRate)
{
    Capture::DeviceRef device = Capture::findDeviceByNameContains( deviceName );
    
    try {
            // some cams may actually work even if not available
            //if( device->checkAvailable() ) 
            {
                return cpp_to_c( *new Capture(width, height, device, frameRate) );
            }
            //else
            //{
                // device is NOT available
            //  return 0;
            //}
        }
        catch( CaptureExc & ) {
            // Unable to initialize device: device->getName() 
            return 0;
        }

    return 0;
}

C_IMPL int getCaptureDevices( CaptureDeviceInfo* result, int forceRefresh )
{
    vector<Capture::DeviceRef> devices( Capture::getDevices( forceRefresh ) );
    int i = 0;
    for( vector<Capture::DeviceRef>::const_iterator deviceIt = devices.begin(); 
            deviceIt != devices.end(); ++deviceIt ) 
    {
        Capture::DeviceRef device = *deviceIt;

        CaptureDeviceInfo *dev = &result[i];

        memcpy(dev->name, device->getName().c_str(), device->getName().size());
        dev->name_size = device->getName().size();
        dev->available = (int)device->checkAvailable();
        dev->connected = (int)device->isConnected();

        ++i;
    }

    return i;
}

C_IMPL void releaseCapture(CCapture *capture)
{
  delete &c_to_cpp(capture);
}

C_IMPL void captureStart(CCapture *capture)
{
    c_to_cpp(capture).start();
}

C_IMPL void captureStop(CCapture *capture)
{
    c_to_cpp(capture).stop();
}

C_IMPL void captureFocusAtPoint(CCapture *capture, float x, float y)
{
    c_to_cpp(capture).focusAtPoint(x, y);
}

C_IMPL void captureGetStillImage(CCapture *capture, void (*callback)(const uint8_t *data, int32_t size))
{
    c_to_cpp(capture).captureStillImage(callback);
}

C_IMPL const unsigned char* captureGetFrame( CCapture* capture, int* width, int* height, int* stride )
{
    const Capture obj = c_to_cpp(capture);
    const CaptureSurface8u surf = obj.getSurface();
    *width = surf.getWidth();
    *height = surf.getHeight();
    *stride = surf.getRowBytes();
    return surf.getData();
}

C_IMPL void captureGetSize( CCapture* capture, int* width, int* height )
{
    const Capture obj = c_to_cpp(capture);
    *width = obj.getWidth();
    *height = obj.getHeight();
}

C_IMPL int captureIsCapturing(CCapture *capture)
{
    return (c_to_cpp(capture).isCapturing());
}

C_IMPL int captureCheckNewFrame(CCapture *capture)
{
    return (c_to_cpp(capture).checkNewFrame());
}

C_IMPL int captureCheckResponse( CCapture* capture )
{
    return (c_to_cpp(capture).checkResponse());
}

C_IMPL int captureSupportsSaveToCameraRoll()
{
    return (Capture::supportsSaveToCameraRoll());
}

C_IMPL int captureSaveToCameraRoll( const char *fileName, const uint8_t* data, int size )
{
    return (Capture::saveToCameraRoll(fileName, data, size));
}

