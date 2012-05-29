#ifndef CAPTURE_H
#define CAPTURE_H

#include "CaptureConfig.h"
#include "CaptureSurface.h"

#if defined( _ENV_MAC_ )
#    if defined( __OBJC__ )
        @class CaptureImplQtKit;
        @class QTCaptureDevice;
#    else
        class CaptureImplQtKit;
        class QTCaptureDevice;
#    endif
#elif defined( _ENV_COCOA_TOUCH_ )
#    if defined( __OBJC__ )
        @class CaptureImplAvFoundation;
#    else
        class CaptureImplAvFoundation;
#    endif
#endif

#include <vector>
#include <string>

#if defined( _ENV_MSW_ )
    class CaptureImplDirectShow;
#elif defined (__ANDROID__)
    class CaptureImplAndroid;
#endif

class Capture 
{
 public:
    class Device;
    typedef std::shared_ptr<Device> DeviceRef;
    
    Capture() {}
    Capture( int32_t width, int32_t height, const DeviceRef device = DeviceRef(), int32_t frameRate = 0 );
    ~Capture() {}

    void        start();
    void        stop();
    void        focusAtPoint(float x, float y);
    
    void        captureStillImage(void (*callback)(const uint8_t *data, int32_t size));

    void        stillImageCallback(const uint8_t *data, int32_t size);
    
    bool        isCapturing();
    
    bool        checkResponse();

    bool        checkNewFrame() const;

    int32_t             getWidth() const;
    int32_t             getHeight() const;
    void                getSize(int32_t &w, int32_t &h) { w=getWidth(); h=getHeight(); }
    CaptureSurface8u    getSurface() const;

    const Capture::DeviceRef getDevice() const;

    static const std::vector<DeviceRef>&    getDevices( bool forceRefresh = false );
    static DeviceRef                        findDeviceByName( const std::string &name );
    static DeviceRef                        findDeviceByNameContains( const std::string &nameFragment );
    static bool                             supportsSaveToCameraRoll();
    static bool                             saveToCameraRoll(const char* fileName, const uint8_t *data, int32_t size);

#if defined( _ENV_COCOA_ )
    typedef std::string DeviceIdentifier;
#else
    typedef int DeviceIdentifier;
#endif

    class Device 
    {
     public:
        virtual ~Device() {}
        const std::string&                    getName() const { return mName; }
        virtual bool                        checkAvailable() const = 0;
        virtual bool                        isConnected() const = 0;
        virtual Capture::DeviceIdentifier    getUniqueId() const = 0;
     protected:
        Device() {}
        std::string        mName;
    };
        
 protected: 
    struct Obj {
        Obj( int32_t width, int32_t height, const Capture::DeviceRef device, int32_t frameRate );
        virtual ~Obj();

#if defined( _ENV_MAC_ ) 
        CaptureImplQtKit                *mImpl;
#elif defined( _ENV_COCOA_TOUCH_ )
        CaptureImplAvFoundation         *mImpl;
#elif defined( _ENV_MSW_ )
        CaptureImplDirectShow           *mImpl;
#elif defined(__ANDROID__)
        CaptureImplAndroid              *mImpl;
#endif
    };
    
    std::shared_ptr<Obj>                mObj;
    
  public:
    typedef std::shared_ptr<Obj> Capture::*unspecified_bool_type;
    operator unspecified_bool_type() const { return ( mObj.get() == 0 ) ? 0 : &Capture::mObj; }
    void reset() { mObj.reset(); }
};

class CaptureExc : public std::exception {
    virtual const char* what() const throw() {
        return "Capture exception";
    }
};

class CaptureExcInitFail : public CaptureExc {
};

#endif
