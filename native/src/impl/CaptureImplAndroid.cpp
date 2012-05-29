
#include <android/log.h>
#include <math.h>

#if !defined(LOGD) && !defined(LOGI) && !defined(LOGE)
#define LOG_TAG "CAPTURE"
#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))
#endif

#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>

using namespace std;

class AndroidCameraActivity;

#include "CaptureImplAndroid.h"
#include "../ColorConvert.h"

class AndroidCameraActivity : public CameraActivity
{
public:
    AndroidCameraActivity(CaptureImplAndroid* capture)
    {
        m_capture = capture;
        m_framesReceived = 0;
    }

    virtual bool onFrameBuffer(void* buffer, int bufferSize)
    {
        if(isConnected() && buffer != 0 && bufferSize > 0)
        {
            m_framesReceived++;
            if (m_capture->m_waitingNextFrame || m_capture->m_shouldAutoGrab)
            {
                pthread_mutex_lock(&m_capture->m_nextFrameMutex);

                m_capture->setFrame(buffer, bufferSize);

                //pthread_cond_broadcast(&m_capture->m_nextFrameCond); // to grab in sync
                pthread_mutex_unlock(&m_capture->m_nextFrameMutex);
            }
            return true;
        }
        return false;
    }

    void LogFramesRate()
    {
        LOGI("FRAMES received: %d  grabbed: %d", m_framesReceived, m_capture->m_framesGrabbed);
    }

private:
    CaptureImplAndroid* m_capture;
    int m_framesReceived;
};

bool CaptureImplAndroid::Device::checkAvailable() const
{
    return true;
}

bool CaptureImplAndroid::Device::isConnected() const
{
    return true;
}

bool CaptureImplAndroid::sDevicesEnumerated = false;
vector<Capture::DeviceRef> CaptureImplAndroid::sDevices;


const vector<Capture::DeviceRef>& CaptureImplAndroid::getDevices( bool forceRefresh )
{
    if( sDevicesEnumerated && ( ! forceRefresh ) )
        return sDevices;

    sDevices.clear();

    sDevices.push_back( Capture::DeviceRef( new CaptureImplAndroid::Device( std::string((const char*)"Back Camera"), 0 ) ) );
    sDevices.push_back( Capture::DeviceRef( new CaptureImplAndroid::Device( std::string((const char*)"Front Camera"), 1 ) ) );

    sDevicesEnumerated = true;
    return sDevices;
}

bool CaptureImplAndroid::saveToCameraRoll(const char* fileName, const uint8_t *data, int32_t size)
{
    FILE *ft;
    char gallery_path[250] = "/sdcard/DCIM/";

    strcat(gallery_path, fileName);

    ft = fopen(gallery_path, "wb");
    if (!ft)
    {
        LOGI("Unable to open file!");
        return false;
    }
    fwrite(data, size, 1, ft);
    fclose(ft);

    return true;
}

void CaptureImplAndroid::traceDir(const std::string& folderPath)
{
    DIR *dp;
    struct dirent *ep;

    LOGI("LIST FOLDER CONTENT");

    dp = opendir (folderPath.c_str());
    if (dp != NULL)
    {
        while ((ep = readdir (dp))) {
            const char* cur_name=ep->d_name;
            //if (strstr(cur_name, PREFIX_CAMERA_WRAPPER_LIB)) {
                //listLibs.push_back(cur_name);
                LOGI("||%s", cur_name);
            //}
        }
        (void) closedir (dp);
    } else 
    {
        LOGI("LIST FOLDER CONTENT: ERROR OPEN DIR");
    }
}

CaptureImplAndroid::CaptureImplAndroid( int32_t width, int32_t height, const Capture::DeviceRef device, int32_t frameRate )
{
    mDevice = device;
    open(device->getUniqueId());
    if(mIsCapturing)
    {
        setFrameSize(width, height);
        
        m_width = m_activity->getFrameWidth();
        m_height = m_activity->getFrameHeight();
        m_frameRate = frameRate;

        //allocate memory if needed
        prepareCacheForYUV(m_width, m_height);

        if (m_frameFormat == noformat)
        {
            union {double prop; const char* name;} u;
            u.prop = (double)m_activity->getProperty(ANDROID_CAMERA_PROPERTY_PREVIEW_FORMAT_STRING);
            if (0 == strcmp(u.name, "yuv420sp"))
                m_frameFormat = yuv420sp;
            else if (0 == strcmp(u.name, "yvu420sp"))
                m_frameFormat = yvu420sp;
            else
                m_frameFormat = yuvUnknown;
        }
    }
}

void CaptureImplAndroid::open(int cameraId)
{
    //defaults
    m_width               = 0;
    m_height              = 0;
    m_activity            = 0;
    m_isOpened            = false;
    m_frameYUV420         = 0;
    m_frameYUV420next     = 0;
    m_hasGray             = false;
    m_hasColor            = false;
    m_dataState           = CVCAPTURE_ANDROID_STATE_NO_FRAME;
    m_waitingNextFrame    = false;
    m_shouldAutoGrab      = false;
    m_framesGrabbed       = 0;
    m_CameraParamsChanged = false;
    m_frameFormat         = noformat;
    mIsCapturing          = false;

    //try connect to camera
    m_activity = new AndroidCameraActivity(this);

    if (m_activity == 0) return;

    pthread_mutex_init(&m_nextFrameMutex, NULL);
    //pthread_cond_init (&m_nextFrameCond,  NULL);

    CameraActivity::ErrorCode errcode = m_activity->connect(cameraId);

    if(errcode == CameraActivity::NO_ERROR)
    {
        m_isOpened = true;
        mIsCapturing = true;
    }
    else
    {
        LOGE("Native_camera returned opening error: %d", errcode);
        delete m_activity;
        m_activity = 0;
    }
}

bool CaptureImplAndroid::isOpened() const
{
    return m_isOpened;
}

CaptureImplAndroid::~CaptureImplAndroid()
{
    if (m_activity)
    {
        ((AndroidCameraActivity*)m_activity)->LogFramesRate();


        pthread_mutex_lock(&m_nextFrameMutex);

        unsigned char *tmp1=m_frameYUV420;
        unsigned char *tmp2=m_frameYUV420next;
        m_frameYUV420 = 0;
        m_frameYUV420next = 0;
        delete tmp1;
        delete tmp2;

        mCurrentFrame.reset();

        m_dataState = CVCAPTURE_ANDROID_STATE_NO_FRAME;
        //pthread_cond_broadcast(&m_nextFrameCond);

        pthread_mutex_unlock(&m_nextFrameMutex);

        //m_activity->disconnect() will be automatically called inside destructor;
        delete m_activity;
        m_activity = 0;

        pthread_mutex_destroy(&m_nextFrameMutex);
        //pthread_cond_destroy(&m_nextFrameCond);
    }
}

// needed to parse android properties
std::vector<std::string> &CaptureImplAndroid::splitString(const std::string &s, char delim, std::vector<std::string> &elems) 
{
    std::stringstream ss(s);
    std::string item;
    while(std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

std::vector<std::string> CaptureImplAndroid::splitString(const std::string &s, char delim) 
{
    std::vector<std::string> elems;
    return CaptureImplAndroid::splitString(s, delim, elems);
}

void CaptureImplAndroid::setFrameSize(int width, int height)
{
    union {double prop; const char* name;} u;
    u.prop = m_activity->getProperty(ANDROID_CAMERA_PROPERTY_SUPPORTED_PREVIEW_SIZES_STRING);

    int best_w = width;
    int best_h = height;
    double minDiff = 10000000.0;
    
    std::vector<std::string> sizes_str = splitString(u.name, ',');
    for(std::vector<std::string>::iterator it = sizes_str.begin(); it != sizes_str.end(); ++it) 
    {
        std::vector<std::string> wh_str = splitString(*it, 'x');
        double _w = ::atof(wh_str[0].c_str());
        double _h = ::atof(wh_str[1].c_str());
        if (std::abs(_h - height) < minDiff) 
        {
            best_w = (int)_w;
            best_h = (int)_h;
            minDiff = std::abs(_h - height);
        }
    }

    m_activity->setProperty(ANDROID_CAMERA_PROPERTY_FRAMEWIDTH, best_w);
    m_activity->setProperty(ANDROID_CAMERA_PROPERTY_FRAMEHEIGHT, best_h);
    //
    m_activity->applyProperties();
    m_CameraParamsChanged = false;
    m_dataState = CVCAPTURE_ANDROID_STATE_NO_FRAME; //we will wait new frame
}

bool CaptureImplAndroid::grabFrame() 
{
    if( !isOpened() ) {
        LOGE("CaptureAndroid::grabFrame(): camera is not opened");
        return false;
    }

    bool res = false;
    pthread_mutex_lock(&m_nextFrameMutex);
    if (m_CameraParamsChanged)
    {
        m_activity->applyProperties();
        m_CameraParamsChanged = false;
        m_dataState = CVCAPTURE_ANDROID_STATE_NO_FRAME; //we will wait new frame
    }

    if (m_dataState != CVCAPTURE_ANDROID_STATE_HAS_NEW_FRAME_UNGRABBED) 
    {
        m_waitingNextFrame = true;
        // grab frames in sync
        //pthread_cond_wait(&m_nextFrameCond, &m_nextFrameMutex);
    }

    if (m_dataState == CVCAPTURE_ANDROID_STATE_HAS_NEW_FRAME_UNGRABBED) 
    {
        //swap current and new frames
        unsigned char* tmp = m_frameYUV420;
        m_frameYUV420 = m_frameYUV420next;
        m_frameYUV420next = tmp;

        //discard cached frames
        m_hasGray = false;
        m_hasColor = false;

        m_dataState = CVCAPTURE_ANDROID_STATE_HAS_FRAME_GRABBED;
        m_framesGrabbed++;

        res=true;
    } 
    /*else 
    {
        LOGE("CaptureAndroid::grabFrame: NO new frame");
    }*/


    int res_unlock = pthread_mutex_unlock(&m_nextFrameMutex);

    if (res_unlock) 
    {
        LOGE("Error in CaptureAndroid::grabFrame: pthread_mutex_unlock returned %d --- probably, this object has been destroyed", res_unlock);
        return false;
    }

    return res;
}

//Attention: this method should be called inside pthread_mutex_lock(m_nextFrameMutex) only
void CaptureImplAndroid::setFrame(const void* buffer, int bufferSize)
{
    //copy data
    memcpy(m_frameYUV420next, buffer, bufferSize);

    m_dataState = CVCAPTURE_ANDROID_STATE_HAS_NEW_FRAME_UNGRABBED;
    m_waitingNextFrame = false;//set flag that no more frames required at this moment
}

void CaptureImplAndroid::prepareCacheForYUV(int width, int height)
{
    //if (width != m_width || height != m_height)
    if (!m_frameYUV420next)
    {
        LOGD("CaptureImplAndroid::prepareCacheForYUV: Changing size of buffers: from width=%d height=%d to width=%d height=%d", m_width, m_height, width, height);
        
        unsigned char *tmp = m_frameYUV420next;
        m_frameYUV420next = new unsigned char [width * height * 3 / 2];
        if (tmp != NULL) {
            delete[] tmp;
        }

        tmp = m_frameYUV420;
        m_frameYUV420 = new unsigned char [width * height * 3 / 2];
        if (tmp != NULL) {
            delete[] tmp;
        }

        mCurrentFrame.reset();
        mCurrentFrame = CaptureSurface8u( width, height, 4 );
    }
}

void CaptureImplAndroid::start()
{
    if( mIsCapturing ) return;
    
    // TODO
    //mWidth = CaptureMgr::instanceVI()->getWidth( mDeviceID );
    //mHeight = CaptureMgr::instanceVI()->getHeight( mDeviceID );

    mIsCapturing = true;
}

void CaptureImplAndroid::stop()
{
    if( ! mIsCapturing ) return;

    // TODO
    
    mIsCapturing = false;
}

bool CaptureImplAndroid::isCapturing()
{
    return mIsCapturing;
}

bool CaptureImplAndroid::checkResponse()
{
    return true;
}

bool CaptureImplAndroid::checkNewFrame() 
{
    return grabFrame();
}

CaptureSurface8u CaptureImplAndroid::getSurface() 
{
    unsigned char *current_frameYUV420 = m_frameYUV420;
    bool inRGBorder = false;
    bool withAlpha = true;
    int clrDepth = 8;

    // Attention! 
    // all the operations in this function below should occupy less time 
    // than the period between two frames from camera
    if (NULL != current_frameYUV420)
    {
        if (!m_hasGray || !m_hasColor)
        {            
            if (m_frameFormat == yuv420sp)
            {
                convertColor(current_frameYUV420, mCurrentFrame.getData(), 
                            m_width, m_height*3/2,
                            1, withAlpha ? 4 : 3,
                            inRGBorder ? YUV420sp2RGB : YUV420sp2BGR, clrDepth);
            } 
            else if (m_frameFormat == yvu420sp)
            {
                convertColor(current_frameYUV420, mCurrentFrame.getData(), 
                            m_width, m_height*3/2,
                            1, withAlpha ? 4 : 3,
                            inRGBorder ? YUV2RGB_NV21 : YUV2BGR_NV12, clrDepth);
            }

            m_hasGray = true;
            m_hasColor = true;
        }
    }

    return mCurrentFrame;
}

bool CaptureImplAndroid::processFrame(int width, int height, const unsigned char * data, unsigned char * out) 
{
    if (data == 0) return false;
    if (m_frameFormat != yuv420sp && m_frameFormat != yvu420sp) return false;

    int frameSize = width * height;
    uint32_t *rgba = (uint32_t*)out;

    int view_mode = 1;

    if (view_mode == 0) // VIEW_MODE_GRAY
    {
        for (int i = 0; i < frameSize; i++) 
        {
            int y = (0xff & ((int) data[i]));
            rgba[i] = 0xff000000 + (y << 16) + (y << 8) + y;
        }
    } 
    else if (view_mode == 1) // VIEW_MODE_RGBA
    {
        for (int i = 0; i < height; i++)
            for (int j = 0; j < width; j++) {
                int y = (0xff & ((int) data[i * width + j]));
                int u = (0xff & ((int) data[frameSize + (i >> 1) * width + (j & ~1) + 0]));
                int v = (0xff & ((int) data[frameSize + (i >> 1) * width + (j & ~1) + 1]));
                y = y < 16 ? 16 : y;

                int r = lrintf(1.164f * (y - 16) + 1.596f * (v - 128));
                int g = lrintf(1.164f * (y - 16) - 0.813f * (v - 128) - 0.391f * (u - 128));
                int b = lrintf(1.164f * (y - 16) + 2.018f * (u - 128));

                r = r < 0 ? 0 : (r > 255 ? 255 : r);
                g = g < 0 ? 0 : (g > 255 ? 255 : g);
                b = b < 0 ? 0 : (b > 255 ? 255 : b);

                rgba[i * width + j] = 0xff000000 + (b << 16) + (g << 8) + r;
            }
    }

    return true;
}