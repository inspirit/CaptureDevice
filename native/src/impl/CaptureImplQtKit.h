
#include "../CaptureConfig.h"
#include "../CaptureSurface.h"
#include "../Capture.h"

#import <QTKit/QTKit.h>

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


class CaptureImplQtKitDevice : public Capture::Device 
{
 public:
    CaptureImplQtKitDevice( QTCaptureDevice* device );
    ~CaptureImplQtKitDevice() {}
    
    bool                         checkAvailable() const;
    bool                         isConnected() const;
    Capture::DeviceIdentifier    getUniqueId() const { return mUniqueId; }
 private:
    Capture::DeviceIdentifier    mUniqueId;
};

@interface CaptureImplQtKit : NSObject {
    bool                                mIsCapturing;
    QTCaptureSession                    *mCaptureSession;
    QTCaptureDecompressedVideoOutput    *mCaptureDecompressedOutput;
    QTCaptureDeviceInput                *mCaptureDeviceInput;
    
    CVPixelBufferRef                mWorkingPixelBuffer;
    CaptureSurface8u                        mCurrentFrame;
    int32_t                            mWidth, mHeight, mFrameRate;
    NSString                        * mDeviceUniqueId;
    int32_t                            mExposedFrameBytesPerRow;
    int32_t                            mExposedFrameHeight;
    int32_t                            mExposedFrameWidth;
    bool                            mHasNewFrame;
    CFTimeInterval                     mLastTickTime;
    Capture::DeviceRef                mDevice;
}

+ (const std::vector<Capture::DeviceRef>&)getDevices:(BOOL)forceRefresh;

- (id)initWithDevice:(const Capture::DeviceRef)device width:(int)width height:(int)height frameRate:(int)frameRate;
- (void)prepareStartCapture;
- (void)startCapture;
- (void)stopCapture;
- (bool)isCapturing;
- (CaptureSurface8u)getCurrentFrame;
- (bool)checkNewFrame;
- (bool)checkResponse;
- (const Capture::DeviceRef)getDevice;
- (int32_t)getWidth;
- (int32_t)getHeight;
- (int32_t)getCurrentFrameBytesPerRow;
- (int32_t)getCurrentFrameWidth;
- (int32_t)getCurrentFrameHeight;

@end