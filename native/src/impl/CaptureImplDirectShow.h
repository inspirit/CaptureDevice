#ifndef IMPL_DIRECT_SHOW_H
#define IMPL_DIRECT_SHOW_H

#include "../CaptureConfig.h"
#include "../Capture.h"
#include "../CaptureSurface.h"
#include "../3dparty/videoInput/videoInput2.h"

class CaptureImplDirectShow {
 public:
    class Device;

    CaptureImplDirectShow( int32_t width, int32_t height, const Capture::DeviceRef device, int32_t frameRate );
    CaptureImplDirectShow( int32_t width, int32_t height, int32_t frameRate );
    ~CaptureImplDirectShow();
    
    void start();
    void stop();
    
    bool        isCapturing();
    bool        checkResponse();
    bool        checkNewFrame() const;

    int32_t        getWidth() const { return mWidth; }
    int32_t        getHeight() const { return mHeight; }

    CaptureSurface8u            getSurface() const;
    
    const Capture::DeviceRef     getDevice() const { return mDevice; }
    
    static const std::vector<Capture::DeviceRef>&    getDevices( bool forceRefresh = false );

    class Device : public Capture::Device {
       public:
        bool                        checkAvailable() const;
        bool                        isConnected() const;
        Capture::DeviceIdentifier   getUniqueId() const { return mUniqueId; }

        Device( const std::string &name, int uniqueId ) : Capture::Device(), mUniqueId( uniqueId ) { mName = name; }
     protected:
        int                mUniqueId;
    };
 protected:
    void    init( int32_t width, int32_t height, const Capture::Device &device, int32_t frameRate );

    int                                mDeviceID;
    // this maintains a reference to the mgr so that we don't destroy it before
    // the last Capture is destroyed
    std::shared_ptr<class CaptureMgr>    mMgrPtr;
    bool                                mIsCapturing;
    std::shared_ptr<class SurfaceCache>    mSurfaceCache;

    int32_t                mWidth, mHeight, mFrameRate;
    mutable CaptureSurface8u    mCurrentFrame;
    Capture::DeviceRef    mDevice;

    static bool                            sDevicesEnumerated;
    static std::vector<Capture::DeviceRef>    sDevices;
};

#endif
