#ifndef IMPL_ANDROID_H
#define IMPL_ANDROID_H

#include <pthread.h>

#include "../CaptureConfig.h"
#include "../Capture.h"
#include "../CaptureSurface.h"

#include "../../android/jni/camera_activity.hpp"

#include <sstream>
#include <cstdlib>
#include <cmath>

class CaptureImplAndroid
{
public:
    class Device;

    CaptureImplAndroid( int32_t width, int32_t height, const Capture::DeviceRef device, int32_t frameRate );
    ~CaptureImplAndroid();

    bool grabFrame();

    bool isOpened() const;

    void start();
    void stop();
    
    bool        isCapturing();
    bool        checkResponse();
    bool        checkNewFrame();

    int32_t        getWidth() const { return m_width; }
    int32_t        getHeight() const { return m_height; }

    CaptureSurface8u                getSurface();
    
    const Capture::DeviceRef                        getDevice() const { return mDevice; }
    static const std::vector<Capture::DeviceRef>&   getDevices( bool forceRefresh = false );
    static bool                                     saveToCameraRoll(const char* fileName, const uint8_t *data, int32_t size);

    void setFrameSize(int width, int height);

    // some utils needed for Android Capture
    static std::vector<std::string> &splitString(const std::string &s, char delim, std::vector<std::string> &elems);
    static std::vector<std::string> splitString(const std::string &s, char delim);

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

    CameraActivity* m_activity;

    Capture::DeviceRef              mDevice;
    CaptureSurface8u                mCurrentFrame;
    bool                            mIsCapturing;

    static bool                            sDevicesEnumerated;
    static std::vector<Capture::DeviceRef>    sDevices;

    //raw from camera
    int m_width;
    int m_height;
    int m_frameRate;
    unsigned char *m_frameYUV420;
    unsigned char *m_frameYUV420next;

    enum YUVformat
    {
        noformat = 0,
        yuv420sp,
        yvu420sp,
        yuvUnknown
    };

    YUVformat m_frameFormat;

    void setFrame(const void* buffer, int bufferSize);
    void open(int);
    void traceDir(const std::string& folderPath);

private:
    bool m_isOpened;
    bool m_CameraParamsChanged;

    //frames counter for statistics
    int m_framesGrabbed;

    //cached converted frames
    bool m_hasGray;
    bool m_hasColor;

    enum Capture_Android_DataState 
    {
        CVCAPTURE_ANDROID_STATE_NO_FRAME=0,
        CVCAPTURE_ANDROID_STATE_HAS_NEW_FRAME_UNGRABBED,
        CVCAPTURE_ANDROID_STATE_HAS_FRAME_GRABBED
    };
    volatile Capture_Android_DataState m_dataState;

    //synchronization
    pthread_mutex_t m_nextFrameMutex;
    pthread_cond_t m_nextFrameCond;
    volatile bool m_waitingNextFrame;
    volatile bool m_shouldAutoGrab;

    void prepareCacheForYUV(int width, int height);
    bool processFrame(int width, int height, const unsigned char * data, unsigned char * out);

    friend class AndroidCameraActivity;
};

#endif
