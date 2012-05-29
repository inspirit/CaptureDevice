
#import "CaptureImplQtKit.h"

#if defined( _ENV_MAC_ )
    #import <Cocoa/Cocoa.h>
    #import <CoreVideo/CVPixelBuffer.h>
    #import <AppKit/AppKit.h>
#else
    #import <UIKit/UIKit.h>
    #import <CoreText/CoreText.h>
#endif

#import <Foundation/NSData.h>

using namespace std;


#ifndef QTKIT_VERSION_7_6_3
#define QTKIT_VERSION_7_6_3         70603
#define QTKIT_VERSION_7_0           70000
#endif
#ifndef QTKIT_VERSION_MAX_ALLOWED
#define QTKIT_VERSION_MAX_ALLOWED QTKIT_VERSION_7_6_3
#endif

std::string    convertNsString( NSString *str )
{
    return std::string( [str UTF8String] );
}

CaptureImplQtKitDevice::CaptureImplQtKitDevice( QTCaptureDevice* device )
    : Capture::Device()
{
    mUniqueId = convertNsString( [device uniqueID] );
    mName = convertNsString( [device localizedDisplayName] );
}

bool CaptureImplQtKitDevice::checkAvailable() const
{
    QTCaptureDevice *device = [QTCaptureDevice deviceWithUniqueID:[NSString stringWithUTF8String:mUniqueId.c_str()]];
    return [device isConnected] && (! [device isInUseByAnotherApplication]);
}

bool CaptureImplQtKitDevice::isConnected() const
{
    QTCaptureDevice *device = [QTCaptureDevice deviceWithUniqueID:[NSString stringWithUTF8String:mUniqueId.c_str()]];
    return [device isConnected];
}

static void frameDeallocator( void *refcon );

static std::vector<Capture::DeviceRef> sDevices;
static BOOL sDevicesEnumerated = false;

@implementation CaptureImplQtKit


+ (const std::vector<Capture::DeviceRef>&)getDevices:(BOOL)forceRefresh
{
    if( sDevicesEnumerated && ( ! forceRefresh ) ) {
        return sDevices;
    }

    sDevices.clear();    

    NSArray *devices = [QTCaptureDevice inputDevicesWithMediaType:QTMediaTypeVideo];
    for( int i = 0; i < [devices count]; i++ ) {
        QTCaptureDevice *device = [devices objectAtIndex:i];
        sDevices.push_back( Capture::DeviceRef( new CaptureImplQtKitDevice( device ) ) );
    }
    
    devices = [QTCaptureDevice inputDevicesWithMediaType:QTMediaTypeMuxed];
    for( int i = 0; i < [devices count]; i++) {
        QTCaptureDevice *device = [devices objectAtIndex:i];
        sDevices.push_back( Capture::DeviceRef( new CaptureImplQtKitDevice( device ) ) );
    }

    sDevicesEnumerated = true;
    return sDevices;
}

- (id)initWithDevice:(const Capture::DeviceRef)device width:(int)width height:(int)height frameRate:(int)frameRate 
{
    if( ( self = [super init] ) ) {

        mDevice = device;
        if( mDevice ) {
            mDeviceUniqueId = [NSString stringWithUTF8String:device->getUniqueId().c_str()];
            [mDeviceUniqueId retain];
        }
        
        mIsCapturing = false;
        mWidth = width;
        mHeight = height;
        mFrameRate = frameRate;
        mHasNewFrame = false;
        mExposedFrameBytesPerRow = 0;
        mExposedFrameWidth = 0;
        mExposedFrameHeight = 0;
        mLastTickTime = CFAbsoluteTimeGetCurrent();
    }
    return self;
}

- (void)dealloc 
{
    if( mIsCapturing ) {
        [self stopCapture];
    }
    
    [mDeviceUniqueId release];
    
    [super dealloc];
}


- (void)prepareStartCapture
{
    mCaptureSession = [[QTCaptureSession alloc] init];
    
    BOOL success = NO;
    NSError *error;
    
    //if mDevice is NULL, use the default video device to capture from
    QTCaptureDevice *device = nil;
    if( ! mDeviceUniqueId ) {
        device = [QTCaptureDevice defaultInputDeviceWithMediaType:QTMediaTypeVideo];
    } else {
        device = [QTCaptureDevice deviceWithUniqueID:mDeviceUniqueId];
    }
    
    if( ! device ) {
        throw CaptureExcInitFail();
    }

    success = [device open:&error];
    if( ! success ) {
        throw CaptureExcInitFail();
    }
    
    mCaptureDeviceInput = [[QTCaptureDeviceInput alloc] initWithDevice:device];
    success = [mCaptureSession addInput:mCaptureDeviceInput error:&error];
    if( ! success ) {
        throw CaptureExcInitFail();
    }
    
    // Disable the sound connection
    NSArray *connections = [mCaptureDeviceInput connections];
    for( int i = 0; i < [connections count]; i++ ) {
        QTCaptureConnection *connection = [connections objectAtIndex:i];
        if( [[connection mediaType] isEqualToString:QTMediaTypeSound] ) {
            [connection setEnabled:NO];
        }
    }
    
    //get decompressed video output
    mCaptureDecompressedOutput = [[QTCaptureDecompressedVideoOutput alloc] init];
    success = [mCaptureSession addOutput:mCaptureDecompressedOutput error:&error];
    if( ! success ) {
        throw CaptureExcInitFail();
    }
    
    [mCaptureDecompressedOutput setDelegate: self];
    
/*    int pixelBufferFormat = cinder::cocoa::getCvPixelFormatTypeFromSurfaceChannelOrder( cinder::SurfaceChannelOrder( mSurfaceChannelOrderCode ) ); 
    if( pixelBufferFormat < 0 ) 
        throw cinder::CaptureExcInvalidChannelOrder();     */
    
    NSDictionary *attributes = [NSDictionary dictionaryWithObjectsAndKeys:
                                [NSNumber numberWithDouble:mWidth], (id)kCVPixelBufferWidthKey,
                                [NSNumber numberWithDouble:mHeight], (id)kCVPixelBufferHeightKey,
                                //10.4: k32ARGBPixelFormat
                                //10.5: kCVPixelFormatType_32ARGB
                                //[NSNumber numberWithUnsignedInt:pixelBufferFormat], (id)kCVPixelBufferPixelFormatTypeKey,
                                [NSNumber numberWithUnsignedInt:kCVPixelFormatType_32BGRA], (id)kCVPixelBufferPixelFormatTypeKey,
                                nil
                                ];
    
    [mCaptureDecompressedOutput setPixelBufferAttributes: attributes];
    
#if QTKIT_VERSION_MAX_ALLOWED >= QTKIT_VERSION_7_6_3
    [mCaptureDecompressedOutput setAutomaticallyDropsLateVideoFrames:YES]; 
#endif
    if (0.0f < mFrameRate && [mCaptureDecompressedOutput respondsToSelector:@selector(setMinimumVideoFrameInterval:)])
		[(id)mCaptureDecompressedOutput setMinimumVideoFrameInterval:(1.0f / (float)mFrameRate)];
}

- (void)startCapture 
{
    if( mIsCapturing )
        return; 

    @synchronized( self ) {
        [self prepareStartCapture];
        
        mWorkingPixelBuffer = 0;
        mHasNewFrame = false;
        mLastTickTime = CFAbsoluteTimeGetCurrent();
        
        mIsCapturing = true;
        [mCaptureSession startRunning];
    }
}

- (void)stopCapture
{
    if( ! mIsCapturing )
        return;
    
    @synchronized( self ) 
    {
        if([mCaptureSession isRunning])[mCaptureSession stopRunning];
        if ([[mCaptureDeviceInput device] isOpen]) [[mCaptureDeviceInput device] close];
        
        [mCaptureSession release];
        [mCaptureDeviceInput release];
        
        mCaptureDeviceInput = nil;
        mCaptureSession = nil;
        
        [mCaptureDecompressedOutput setDelegate:mCaptureDecompressedOutput]; 
        [mCaptureDecompressedOutput release];
        mCaptureDecompressedOutput = nil;
        

        mIsCapturing = false;
        mHasNewFrame = false;
        
        mCurrentFrame.reset();
        
        if( mWorkingPixelBuffer ) {
            CVBufferRelease( mWorkingPixelBuffer );
            mWorkingPixelBuffer = 0;
        }
    }
}

- (bool)isCapturing
{
    return mIsCapturing;
}

- (CaptureSurface8u)getCurrentFrame
{
    if( ( ! mIsCapturing ) || ( ! mWorkingPixelBuffer ) ) {
        return mCurrentFrame;
    }
    
    @synchronized (self) {
        CVPixelBufferLockBaseAddress( mWorkingPixelBuffer, 0 );
        
        uint8_t *data = (uint8_t *)CVPixelBufferGetBaseAddress( mWorkingPixelBuffer );
        mExposedFrameBytesPerRow = CVPixelBufferGetBytesPerRow( mWorkingPixelBuffer );
        mExposedFrameWidth = CVPixelBufferGetWidth( mWorkingPixelBuffer );
        mExposedFrameHeight = CVPixelBufferGetHeight( mWorkingPixelBuffer );

        // RGB
        mCurrentFrame = CaptureSurface8u( data, mExposedFrameWidth, mExposedFrameHeight, mExposedFrameBytesPerRow );
        mCurrentFrame.setDeallocator( frameDeallocator, mWorkingPixelBuffer );
        
        // mark the working pixel buffer as empty since we have wrapped it in the current frame
        mWorkingPixelBuffer = 0;
    }
    
    return mCurrentFrame;
}

- (bool)checkNewFrame
{
    if (!mIsCapturing) {
        return false;
    }
    
    bool result;
    @synchronized (self) {
        result = mHasNewFrame;
        mHasNewFrame = false;
    }
    return result;
}

- (bool)checkResponse
{
    if (!mIsCapturing) {
        return true;
    }
    
    float deltaTimeInSeconds = CFAbsoluteTimeGetCurrent() - mLastTickTime;
    
    if(deltaTimeInSeconds > 4)
    {
        return false;
    }
    
    return true;
}

void frameDeallocator( void *refcon )
{
    CVPixelBufferRef pixelBuffer = reinterpret_cast<CVPixelBufferRef>( refcon );
    CVPixelBufferUnlockBaseAddress( pixelBuffer, 0 );
    CVBufferRelease( pixelBuffer );
}

- (const Capture::DeviceRef)getDevice
{
    return mDevice;
}

- (int32_t)getWidth
{
    return mWidth;
}

- (int32_t)getHeight
{
    return mHeight;
}

- (int32_t)getCurrentFrameBytesPerRow
{
    return mExposedFrameBytesPerRow;
}

- (int32_t)getCurrentFrameWidth
{
    return mExposedFrameWidth;
}

- (int32_t)getCurrentFrameHeight
{
    return mExposedFrameHeight;
}

- (void)captureOutput:(QTCaptureOutput *)captureOutput didOutputVideoFrame:(CVImageBufferRef)videoFrame withSampleBuffer:(QTSampleBuffer *)sampleBuffer fromConnection:(QTCaptureConnection *)connection
{
    @synchronized( self ) 
    {
        if( mIsCapturing ) 
        {
            // if the last pixel buffer went unclaimed, we'll need to release it
            if( mWorkingPixelBuffer ) {
                CVBufferRelease( mWorkingPixelBuffer );
            }
            
            CVBufferRetain( videoFrame );
        
            mWorkingPixelBuffer = (CVPixelBufferRef)videoFrame;
            mHasNewFrame = true;
            mLastTickTime = CFAbsoluteTimeGetCurrent();
        }
    }    
}

- (void)captureOutput:(QTCaptureOutput *)captureOutput 
didDropVideoFrameWithSampleBuffer:(QTSampleBuffer *)sampleBuffer 
fromConnection:(QTCaptureConnection *)connection {
    //cout << "Camera dropped frame!" << endl; 
}

@end
