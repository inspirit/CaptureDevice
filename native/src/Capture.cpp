//
// ORIGINAL CODE GRABBED FROM CINDER FRAMEWORK
// WITH SOME SIMPLIFICATION/OPTIMIZATION/CLARIFICATION
//

#include "CaptureConfig.h"
#include "Capture.h"

#if defined( _ENV_MAC_ )
    #import "impl/CaptureImplQtKit.h"
    typedef CaptureImplQtKit    CapturePlatformImpl;
#elif defined( _ENV_MSW_ )
    #include "impl/CaptureImplDirectShow.h"
    typedef CaptureImplDirectShow    CapturePlatformImpl;
#elif defined( _ENV_COCOA_TOUCH_ )
    #include "impl/CaptureImplAvFoundation.h"
    typedef CaptureImplAvFoundation    CapturePlatformImpl;
#elif defined( __ANDROID__ )
    #include "impl/CaptureImplAndroid.h"
    typedef CaptureImplAndroid    CapturePlatformImpl;
#endif

#include <set>
using namespace std;

const vector<Capture::DeviceRef>& Capture::getDevices( bool forceRefresh )
{    
#if defined( _ENV_COCOA_ )
    return [CapturePlatformImpl getDevices:forceRefresh];
#else
    return CapturePlatformImpl::getDevices( forceRefresh );
#endif
}

Capture::DeviceRef Capture::findDeviceByName( const string &name )
{
    const vector<DeviceRef> &devices = getDevices();
    for( vector<DeviceRef>::const_iterator deviceIt = devices.begin(); deviceIt != devices.end(); ++deviceIt ) {
        if( (*deviceIt)->getName().compare(name) == 0 )
            return *deviceIt;
    }
    
    return DeviceRef(); // failed - return "null" device
}

Capture::DeviceRef Capture::findDeviceByNameContains( const string &nameFragment )
{    
    const vector<DeviceRef> &devices = getDevices();
    for( vector<DeviceRef>::const_iterator deviceIt = devices.begin(); deviceIt != devices.end(); ++deviceIt ) {
        if( (*deviceIt)->getName().find( nameFragment ) != std::string::npos )
            return *deviceIt;
    }

    return DeviceRef();
}
bool Capture::supportsSaveToCameraRoll()
{
#if defined( _ENV_COCOA_TOUCH_ ) || defined (__ANDROID__)
    return true;
#else
    return false;
#endif
}

bool Capture::saveToCameraRoll(const char* fileName, const uint8_t *data, int32_t size)
{
#if defined( _ENV_COCOA_TOUCH_ ) 
    return [CapturePlatformImpl saveToCameraRoll:fileName data:data size:size];
#elif defined (__ANDROID__)
    return CapturePlatformImpl::saveToCameraRoll( fileName, data, size );
#else
    return false;
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Capture::Obj
Capture::Obj::Obj( int32_t width, int32_t height, const DeviceRef device, int32_t frameRate )
{
#if defined( _ENV_COCOA_ )
    mImpl = [[::CapturePlatformImpl alloc] initWithDevice:device width:width height:height frameRate:frameRate];
#else
    mImpl = new CapturePlatformImpl( width, height, device, frameRate );
#endif    
}

Capture::Obj::~Obj()
{
#if defined( _ENV_COCOA_ )
    [((::CapturePlatformImpl*)mImpl) release];
#else
    delete mImpl;
#endif
}

Capture::Capture( int32_t width, int32_t height, const DeviceRef device, int32_t frameRate ) 
{
    mObj = shared_ptr<Obj>( new Obj( width, height, device, frameRate ) );
}

void Capture::start()
{
#if defined( _ENV_COCOA_ )
    [((::CapturePlatformImpl*)mObj->mImpl) startCapture];
#else
    mObj->mImpl->start();
#endif
}

void Capture::stop()
{
#if defined( _ENV_COCOA_ )
    [((::CapturePlatformImpl*)mObj->mImpl) stopCapture];
#else
    mObj->mImpl->stop();
#endif
}

void Capture::focusAtPoint(float x, float y)
{
#if defined( _ENV_COCOA_TOUCH_ ) 
    [((::CapturePlatformImpl*)mObj->mImpl) focusAtPoint:x y:y];
#elif defined (__ANDROID__)
    //
#endif
}

void (*callBack)(const uint8_t *data, int32_t size) = NULL;

void Capture::stillImageCallback(const uint8_t *data, int32_t size)
{
    if(*callBack)
    {
        (*callBack)(data, size);
    }
}

void Capture::captureStillImage(void (*callback)(const uint8_t *data, int32_t size))
{
#if defined( _ENV_COCOA_TOUCH_ )
    callBack = callback;
    [((::CapturePlatformImpl*)mObj->mImpl) captureStillImage:this method:&Capture::stillImageCallback];
#endif
}

bool Capture::isCapturing()
{
#if defined( _ENV_COCOA_ )
    return [((::CapturePlatformImpl*)mObj->mImpl) isCapturing];
#else
    return mObj->mImpl->isCapturing();
#endif
}

bool Capture::checkResponse()
{
#if defined( _ENV_COCOA_ )
    return [((::CapturePlatformImpl*)mObj->mImpl) checkResponse];
#else
    return mObj->mImpl->checkResponse();
#endif
}

bool Capture::checkNewFrame() const
{
#if defined( _ENV_COCOA_ )
    return [((::CapturePlatformImpl*)mObj->mImpl) checkNewFrame];
#else
    return mObj->mImpl->checkNewFrame();
#endif    
}

CaptureSurface8u Capture::getSurface() const
{
#if defined( _ENV_COCOA_ )
    return [((::CapturePlatformImpl*)mObj->mImpl) getCurrentFrame];
#else
    return mObj->mImpl->getSurface();
#endif
}

int32_t    Capture::getWidth() const { 
#if defined( _ENV_COCOA_ )
    return [((::CapturePlatformImpl*)mObj->mImpl) getWidth];
#else 
    return mObj->mImpl->getWidth();
#endif
}

int32_t    Capture::getHeight() const { 
#if defined( _ENV_COCOA_ )
    return [((::CapturePlatformImpl*)mObj->mImpl) getHeight];
#else
    return mObj->mImpl->getHeight();
#endif
}

const Capture::DeviceRef Capture::getDevice() const {
#if defined( _ENV_COCOA_ )
    return [((::CapturePlatformImpl*)mObj->mImpl) getDevice];
#else
    return mObj->mImpl->getDevice();
#endif
}
