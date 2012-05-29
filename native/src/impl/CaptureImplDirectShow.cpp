
#include "CaptureImplDirectShow.h"
#include "../3dparty/noncopyable.hpp"
#include "../3dparty/checked_delete.hpp"

#include <set>
using namespace std;


bool CaptureImplDirectShow::sDevicesEnumerated = false;
vector<Capture::DeviceRef> CaptureImplDirectShow::sDevices;

class CaptureMgr : private noncopyable
{
 public:
    CaptureMgr();
    ~CaptureMgr();

    static std::shared_ptr<CaptureMgr>    instance();
    static videoInput*    instanceVI() { return instance()->mVideoInput; }

    static std::shared_ptr<CaptureMgr>    sInstance;
    static int                        sTotalDevices;
    
 private:    
    videoInput            *mVideoInput;
};
std::shared_ptr<CaptureMgr>    CaptureMgr::sInstance;
int                            CaptureMgr::sTotalDevices = 0;

CaptureMgr::CaptureMgr()
{
    mVideoInput = new videoInput;
    //mVideoInput->setUseCallback( true );
}

CaptureMgr::~CaptureMgr()
{
    delete mVideoInput;
}

std::shared_ptr<CaptureMgr> CaptureMgr::instance()
{
    if( ! sInstance ) {
        sInstance = std::shared_ptr<CaptureMgr>( new CaptureMgr );
    }
    return sInstance;
}

class SurfaceCache {
 public:
    SurfaceCache( int32_t width, int32_t height, int32_t numChannels, int32_t numSurfaces )
        : mWidth( width ), mHeight( height ), mChannels( numChannels )
    {
        for( int i = 0; i < numSurfaces; ++i ) {
            mSurfaceData.push_back( std::shared_ptr<uint8_t>( new uint8_t[width*height*numChannels], checked_array_deleter<uint8_t>() ) );
            mDeallocatorRefcon.push_back( make_pair( this, i ) );
            mSurfaceUsed.push_back( false );
        }
    }
    
    CaptureSurface8u getNewSurface()
    {
        // try to find an available block of pixel data to wrap a surface around    
        for( size_t i = 0; i < mSurfaceData.size(); ++i ) {
            if( ! mSurfaceUsed[i] ) {
                mSurfaceUsed[i] = true;
                CaptureSurface8u result( mSurfaceData[i].get(), mWidth, mHeight, mWidth * mChannels );
                result.setDeallocator( surfaceDeallocator, &mDeallocatorRefcon[i] );
                return result;
            }
        }

        // we couldn't find an available surface, so we'll need to allocate one
        return CaptureSurface8u( mWidth, mHeight, mChannels );
    }
    
    static void surfaceDeallocator( void *refcon )
    {
        pair<SurfaceCache*,int> *info = reinterpret_cast<pair<SurfaceCache*,int>*>( refcon );
        info->first->mSurfaceUsed[info->second] = false;
    }

 private:
    vector<std::shared_ptr<uint8_t> >    mSurfaceData;
    vector<bool>                        mSurfaceUsed;
    vector<pair<SurfaceCache*,int> >    mDeallocatorRefcon;
    int32_t                                mWidth, mHeight, mChannels;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CaptureImplDirectShow

bool CaptureImplDirectShow::Device::checkAvailable() const
{
    return ( mUniqueId < CaptureMgr::sTotalDevices ) && ( ! CaptureMgr::instanceVI()->isDeviceSetup( mUniqueId ) );
}

bool CaptureImplDirectShow::Device::isConnected() const
{
    return CaptureMgr::instanceVI()->isDeviceConnected( mUniqueId );
}

const vector<Capture::DeviceRef>& CaptureImplDirectShow::getDevices( bool forceRefresh )
{
    if( sDevicesEnumerated && ( ! forceRefresh ) )
        return sDevices;

    sDevices.clear();

    CaptureMgr::instance()->sTotalDevices = CaptureMgr::instanceVI()->listDevices( true );
    for( int i = 0; i < CaptureMgr::instance()->sTotalDevices; ++i ) {
        sDevices.push_back( Capture::DeviceRef( new CaptureImplDirectShow::Device( videoInput::getDeviceName( i ), i ) ) );
    }

    sDevicesEnumerated = true;
    return sDevices;
}

CaptureImplDirectShow::CaptureImplDirectShow( int32_t width, int32_t height, const Capture::DeviceRef device, int32_t frameRate )
    : mWidth( width ), mHeight( height ), mDeviceID( 0 ), mFrameRate( frameRate )
{
    mDevice = device;
    if( mDevice ) {
        mDeviceID = device->getUniqueId();
    }

    CoInitialize(0);

    if( ! CaptureMgr::instanceVI()->setupDevice( mDeviceID, mWidth, mHeight ) )
        throw CaptureExcInitFail();
    mWidth = CaptureMgr::instanceVI()->getWidth( mDeviceID );
    mHeight = CaptureMgr::instanceVI()->getHeight( mDeviceID );

    if(mFrameRate > 0) CaptureMgr::instanceVI()->setIdealFramerate(mDeviceID, mFrameRate);

    mIsCapturing = true;
    mSurfaceCache = std::shared_ptr<SurfaceCache>( new SurfaceCache( mWidth, mHeight, 3, 4 ) );

    mMgrPtr = CaptureMgr::instance();
}

CaptureImplDirectShow::~CaptureImplDirectShow()
{
    CaptureMgr::instanceVI()->stopDevice( mDeviceID );
    
    CoUninitialize();
}

void CaptureImplDirectShow::start()
{
    if( mIsCapturing ) return;
    
    if( ! CaptureMgr::instanceVI()->setupDevice( mDeviceID, mWidth, mHeight ) )
        throw CaptureExcInitFail();
    if( ! CaptureMgr::instanceVI()->isDeviceSetup( mDeviceID ) )
        throw CaptureExcInitFail();
    mWidth = CaptureMgr::instanceVI()->getWidth( mDeviceID );
    mHeight = CaptureMgr::instanceVI()->getHeight( mDeviceID );
    if(mFrameRate > 0) CaptureMgr::instanceVI()->setIdealFramerate(mDeviceID, mFrameRate);
    mIsCapturing = true;
}

void CaptureImplDirectShow::stop()
{
    if( ! mIsCapturing ) return;

    CaptureMgr::instanceVI()->stopDevice( mDeviceID );
    mIsCapturing = false;
}

bool CaptureImplDirectShow::isCapturing()
{
    return mIsCapturing;
}

bool CaptureImplDirectShow::checkResponse()
{
    return CaptureMgr::instanceVI()->isDeviceConnected( mDeviceID );
}

bool CaptureImplDirectShow::checkNewFrame() const
{
    return CaptureMgr::instanceVI()->isFrameNew( mDeviceID );
}

CaptureSurface8u CaptureImplDirectShow::getSurface() const
{
    if( CaptureMgr::instanceVI()->isFrameNew( mDeviceID ) ) {
        mCurrentFrame = mSurfaceCache->getNewSurface();
        CaptureMgr::instanceVI()->getPixels( mDeviceID, mCurrentFrame.getData(), false, false );
    }
    
    return mCurrentFrame;
}
