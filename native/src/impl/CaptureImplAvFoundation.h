
#include "../CaptureConfig.h"
#include "../CaptureSurface.h"
#include "../Capture.h"

#import <AVFoundation/AVFoundation.h>
#include <vector>


#if defined( _ENV_COCOA_TOUCH_ )
    #include <CoreGraphics/CoreGraphics.h>
#else
    #include <ApplicationServices/ApplicationServices.h>
#endif

#include <CoreFoundation/CoreFoundation.h>

#if defined( __OBJC__ )
    @class NSBitmapImageRep;
    @class NSString;
    @class NSData;
#else
    class NSBitmapImageRep;
    class NSString;
    class NSData;    
#endif

typedef void (Capture::*imageCallback)(const uint8_t *data, int32_t size);


class CaptureImplAvFoundationDevice : public Capture::Device {
 public:
    CaptureImplAvFoundationDevice( AVCaptureDevice * device );
    ~CaptureImplAvFoundationDevice();
    
    bool                        checkAvailable() const;
    bool                        isConnected() const;
    Capture::DeviceIdentifier   getUniqueId() const { return mUniqueId; }
 private:
    Capture::DeviceIdentifier    mUniqueId;
    AVCaptureDevice            * mNativeDevice;
};


@interface CaptureImplAvFoundation : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate> {
    AVCaptureSession                * mSession;
    AVCaptureDeviceInput            * mCapDeviceInput;
    AVCaptureVideoDataOutput        * mCapOutput;
    AVCaptureStillImageOutput       * photoOutput;
    CVPixelBufferRef                mWorkingPixelBuffer;
    CaptureSurface8u                mCurrentFrame;
    NSString                        * mDeviceUniqueId;
    
    Capture::DeviceRef                mDevice;
    bool                            mHasNewFrame;
    bool                            mIsCapturing;
    int32_t                            mWidth, mHeight, mFrameRate;
    int32_t                            mSurfaceChannelOrderCode;
    int32_t                            mExposedFrameBytesPerRow;
    int32_t                            mExposedFrameHeight;
    int32_t                            mExposedFrameWidth;
    CFTimeInterval                     mLastTickTime;
}

+ (const std::vector<Capture::DeviceRef>&)getDevices:(BOOL)forceRefresh;
+ (bool)saveToCameraRoll:(const char*)name data:(const uint8_t*)data size:(int)size;
+ (AVCaptureConnection *)connectionWithMediaType:(NSString *)mediaType fromConnections:(NSArray *)connections;

- (id)initWithDevice:(const Capture::DeviceRef)device width:(int)width height:(int)height frameRate:(int)frameRate;
- (bool)prepareStartCapture;
- (void)cameraSizeForCameraInput:(AVCaptureDeviceInput*)input;
- (void)startCapture;
- (void)stopCapture;
- (bool)isCapturing;
- (void)captureStillImage:(Capture *)f method:(imageCallback)call;
- (CaptureSurface8u)getCurrentFrame;
- (bool)checkNewFrame;
- (bool)checkResponse;
- (const Capture::DeviceRef)getDevice;
- (int32_t)getWidth;
- (int32_t)getHeight;
- (int32_t)getCurrentFrameBytesPerRow;
- (int32_t)getCurrentFrameWidth;
- (int32_t)getCurrentFrameHeight;
- (void)focusAtPoint:(float)x y:(float)y;

@end