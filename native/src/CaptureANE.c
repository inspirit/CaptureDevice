
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined (__ANDROID__)
#include <android/log.h>
#define  LOG_TAG    "CAPTURE"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGW(...)  __android_log_print(ANDROID_LOG_WARN,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#endif

#include "FlashRuntimeExtensions.h"
#include "CCapture.h"
#include "resize.h"

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
    #define ANE_MSW
    #define ANE_EXPORT __declspec( dllexport )
#elif defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)
    #define ANE_EXPORT __attribute__((visibility("default")))
    #include "TargetConditionals.h"
    #if TARGET_OS_IPHONE
        #define ANE_IOS
    #else
        #define ANE_MAC
    #endif
#else
    #define ANE_EXPORT 
#endif

#ifdef __cplusplus
extern "C" {
#endif

    ANE_EXPORT void captureInitializer(void** extData, FREContextInitializer* ctxInitializer, FREContextFinalizer* ctxFinalizer);
    ANE_EXPORT void captureFinalizer(void* extData);

#ifdef __cplusplus
} // end extern "C"
#endif

// 
// ADOBE AIR NATIVE EXTENSION WRAPPER
//


#define MAX_ACTIVE_CAMS (20)
#define FRAME_BITMAP (1 << 1)
#define FRAME_RAW (1 << 2)
#define FRAME_P2_BGRA (1 << 3)

static size_t active_cams_count = 0;

static CCapture* active_cams[MAX_ACTIVE_CAMS];

static FREContext _ctx = NULL;
static uint8_t *_cam_shot_data = 0;
static int32_t _cam_shot_size = 0;

// resize buffers
static int32_t resize_buf_size = 0;
static uint8_t *resize_buf = 0;

static size_t ba_write_int(void *ptr, int32_t val)
{
    int32_t *mptr = (int32_t*)ptr;
    *mptr = val;
    return sizeof(int32_t);
}
static size_t ba_read_int(void *ptr, int32_t *val)
{
    int32_t *mptr = (int32_t*)ptr;
    *val = (*mptr);
    return sizeof(int32_t);
}

static uint32_t nextPowerOfTwo(uint32_t v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

static __inline__ void* alignPtr(void* ptr, int n)
{
    return (void*)(((size_t)ptr + n-1) & -n);
}

static void* cvAlloc( size_t size)
{
    uint8_t* udata = (uint8_t*)malloc(size + sizeof(void*) + 32);
    if(!udata) return 0;
    uint8_t** adata = (uint8_t**)alignPtr((uint8_t**)udata + 1, 32);
    adata[-1] = udata;
    return adata;
}

static void cvFree( void* ptr )
{
    if(ptr)
    {
        uint8_t* udata = ((uint8_t**)ptr)[-1];
        free(udata);
    }
}

static uint8_t * allocResizeBuf(int32_t w, int32_t h, int32_t ch)
{
    const int32_t sz = w * h * ch;
    if(resize_buf_size < sz)
    {
        if(resize_buf) cvFree(resize_buf);
        resize_buf = (uint8_t*)cvAlloc(sz);
        resize_buf_size = sz;
    }

    return resize_buf;
}

static void dispose_ane()
{
    int32_t i;
    CCapture* cap;
    for(i = 0; i < MAX_ACTIVE_CAMS; i++)
    {
        cap = active_cams[i];

        if(cap)
        {
            releaseCapture(cap);

            active_cams[i] = 0;
        }
    }
    active_cams_count = 0;

    if(resize_buf_size)
    {
        cvFree(resize_buf);
        resize_buf_size = 0;
    }
    
    if(_cam_shot_data) free(_cam_shot_data);
    _ctx = NULL;
}

FREObject listDevices(FREContext ctx, void* funcData, uint32_t argc, FREObject argv[])
{

    int32_t i, numDevices = 0;
    CaptureDeviceInfo devices[MAX_ACTIVE_CAMS * 2];

    int32_t refresh = 0;
    FREGetObjectAsInt32(argv[0], &refresh);

    numDevices = getCaptureDevices(devices, refresh);

    FREObject objectBA = argv[1];
    FREByteArray baData;

    FREAcquireByteArray(objectBA, &baData);

    uint8_t *ba = baData.bytes;

    ba += ba_write_int(ba, numDevices);

    for (i = 0; i < numDevices; i++)
    {
        const CaptureDeviceInfo *dev = &devices[i];

        ba += ba_write_int(ba, dev->name_size);
        memcpy( ba, (uint8_t*)dev->name, dev->name_size ); 
        ba += dev->name_size;

        ba += ba_write_int(ba, dev->available);
        ba += ba_write_int(ba, dev->connected);

        //printf("device (%d): %s  \n", i, dev->name);
    }

    FREReleaseByteArray(objectBA);

    return NULL;
}

FREObject getCapture(FREContext ctx, void* funcData, uint32_t argc, FREObject argv[])
{
    int32_t w, h;
    int32_t frameRate = 0;
    uint32_t name_size = 0;
    const uint8_t* name_val = NULL;

    FREGetObjectAsUTF8(argv[0], &name_size, &name_val);

    FREGetObjectAsInt32(argv[1], &w);
    FREGetObjectAsInt32(argv[2], &h);
    FREGetObjectAsInt32(argv[3], &frameRate);

    FREObject objectBA = argv[4];
    FREByteArray baData;
    FREAcquireByteArray(objectBA, &baData);
    uint8_t *ba = baData.bytes;


    int32_t emptySlot = -1;
    size_t i;
    // search empty slot
    for(i = 0; i < MAX_ACTIVE_CAMS; i++)
    {
        if(!active_cams[i])
        {
            emptySlot = i;
            break;
        }
    }

    if(emptySlot == -1)
    {
        ba += ba_write_int(ba, -1);
        FREReleaseByteArray(objectBA);
        return NULL;
    }

    CCapture* cap = NULL;

    cap = createCameraCapture(w, h, (char *)name_val, frameRate );
    
    if(!cap)
    {
        ba += ba_write_int(ba, -1);
        FREReleaseByteArray(objectBA);
        return NULL;
    }

    // start if not running
    if( captureIsCapturing(cap) == 0 )
    {
        captureStart(cap);
    }

    active_cams[emptySlot] = cap;
    active_cams_count++;

    captureGetSize( cap, &w, &h );

    // write result
    ba += ba_write_int(ba, emptySlot);
    ba += ba_write_int(ba, w);
    ba += ba_write_int(ba, h);

    FREReleaseByteArray(objectBA);

    return NULL;
}

FREObject delCapture(FREContext ctx, void* funcData, uint32_t argc, FREObject argv[])
{
    int32_t _id;
    FREGetObjectAsInt32(argv[0], &_id);


    if(_id < 0 || _id >= MAX_ACTIVE_CAMS)
    {
        return NULL;
    }

    CCapture* cap;
    cap = active_cams[_id];

    if(cap)
    {
        releaseCapture(cap);

        active_cams[_id] = 0;
        active_cams_count--;
    }

    return NULL;
}

FREObject toggleCapturing(FREContext ctx, void* funcData, uint32_t argc, FREObject argv[])
{
    int32_t _id, _opt;
    FREGetObjectAsInt32(argv[0], &_id);
    FREGetObjectAsInt32(argv[1], &_opt);

    CCapture* cap;
    cap = active_cams[_id];

    if(cap)
    {
        if(_opt == 0)
        {
            captureStop(cap);
        }
        else if(_opt == 1)
        {
            // TODO can fail when starting after stop so we need to handle result
            captureStart(cap);
        }
    }

    return NULL;
}

FREObject getCaptureFrame(FREContext ctx, void* funcData, uint32_t argc, FREObject argv[])
{
    int32_t _id, _opt;
    FREGetObjectAsInt32(argv[0], &_id);

    FREObject res_obj;
    FRENewObjectFromInt32(0, &res_obj);

    FREObject objectBA;
    FREByteArray baData;
    FREBitmapData2 bitmapData;
    uint8_t *ba;
    const uint8_t* frame_0;
    const uint8_t* frame;

    int32_t w, h, w2, h2, widthInBytes, i, j;
    uint32_t fstride;
    CCapture* cap;
    cap = active_cams[_id];
    
    uint32_t flipped = 0;
    uint32_t isWin = 1;
    #if defined(ANE_MAC)
        isWin = 0;
    #endif

    objectBA = argv[1];
    FREAcquireByteArray(objectBA, &baData);
    ba = baData.bytes;

    ba += ba_read_int(ba, &_opt);
    ba += ba_read_int(ba, &w2);
    ba += ba_read_int(ba, &h2);

    FREReleaseByteArray(objectBA);

    if(cap && captureCheckNewFrame(cap))
    {
        frame_0 = (const uint8_t*)captureGetFrame(cap, &w, &h, &widthInBytes);

        // here is some conversion we need cause in Flash BitmapData bytes
        // represented differently :(
        if((_opt & FRAME_BITMAP))
        {
            objectBA = argv[2];
            FREAcquireBitmapData2(objectBA, &bitmapData);

            uint32_t* input = bitmapData.bits32;
            fstride = bitmapData.lineStride32 * sizeof(uint32_t); // for some reasons not always == width
            flipped = bitmapData.isInvertedY;

            frame = frame_0;
            ba = (uint8_t*)input;
            
            #if defined (__ANDROID__) || defined (ANE_IOS)

                for(i=0; i<h; i++){
                    memcpy(ba+i*fstride, frame+i*widthInBytes, fstride);
                }

            #else

            // most likely wont happen / but who knows? ;)
            if(flipped && !isWin)
            {
                const uint8_t* a_ptr = (const uint8_t*)frame;
                const uint8_t* a_ptr2 = (const uint8_t*)(frame + (h - 1) * widthInBytes);
                uint8_t* b_ptr = ba + (h - 1) * fstride;
                uint8_t* b_ptr2 = ba;
                for(i = 0; i < h / 2; i++)
                {
                    uint8_t *ba_ln = b_ptr;
                    uint8_t *ba_ln2 = b_ptr2;
                    const uint8_t *a_ln = a_ptr;
                    const uint8_t *a_ln2 = a_ptr2;
                    #if defined(ANE_MSW)
                    for(j = 0; j < w; j++, ba_ln += 4, ba_ln2 += 4, a_ln += 3, a_ln2 += 3)
                    {
                            *ba_ln = *(a_ln);
                            ba_ln[1] = *(a_ln+1);
                            ba_ln[2] = *(a_ln+2);
                            //
                            *ba_ln2 = *(a_ln2);
                            ba_ln2[1] = *(a_ln2+1);
                            ba_ln2[2] = *(a_ln2+2);
                    }
                    #elif defined(ANE_MAC)
                    memcpy(ba_ln, a_ln, widthInBytes);
                    memcpy(ba_ln2, a_ln2, widthInBytes);
                    #endif
                    
                    a_ptr += widthInBytes;
                    a_ptr2 -= widthInBytes;
                    b_ptr -= fstride;
                    b_ptr2 += fstride;
                }
                if(h&1)
                {
                    #if defined(ANE_MSW)
                        for(j = 0; j < w; j++, b_ptr += 4, a_ptr+=3)
                        {
                            *b_ptr = *(a_ptr);
                            b_ptr[1] = *(a_ptr+1);
                            b_ptr[2] = *(a_ptr+2);
                        }
                    #elif defined(ANE_MAC)
                        memcpy(b_ptr, a_ptr, widthInBytes);
                    #endif
                }
            }
            else
            {
                for(i = 0; i < h; i++)
                {
                    uint8_t *ba_ln = ba;
                    const uint8_t *fr_ln = frame;
                    #if defined(ANE_MSW)
                        for(j = 0; j < w; j++, ba_ln += 4, fr_ln += 3)
                        {                        
                            *ba_ln = *(fr_ln);
                            ba_ln[1] = *(fr_ln+1);
                            ba_ln[2] = *(fr_ln+2);
                        }
                    #elif defined(ANE_MAC) 
                        memcpy(ba_ln, fr_ln, widthInBytes);
                    #endif
                    ba += fstride;
                    frame += widthInBytes;
                }
            }

            #endif


            FREReleaseBitmapData(objectBA);
        } 

        if((_opt & FRAME_RAW))
        {
            objectBA = argv[3];
            FREAcquireByteArray(objectBA, &baData);
            ba = baData.bytes;

            memcpy(ba, frame_0, widthInBytes * h);

            FREReleaseByteArray(objectBA);
        }

        // power of 2 output for stage3d
        if((_opt & FRAME_P2_BGRA))
        {
            objectBA = argv[4];
            FREAcquireByteArray(objectBA, &baData);
            ba = baData.bytes;

            frame = frame_0;

            // resize
            /*
            if(w > w2 || h > h2)
            {
                float scx = (float)w2 / (float)w;
                float scy = (float)h2 / (float)h;
                if(scy < scx) scx = scy;
                int32_t wr = w * scx;
                int32_t hr = h * scx;
                uint8_t *frame_rs = allocResizeBuf(wr, hr, 4);
                //resample_area_8u(frame, w, h, frame_rs, wr, hr, 4);

                // update values
                w = wr;
                h = hr;
                frame = frame_rs;
                widthInBytes = wr * sizeof(uint32_t);
            }
            */

            int32_t p2w = (w2);
            int32_t p2h = (h2);
            int32_t off_x = (p2w - w) / 2; // -64
            int32_t off_y = (p2h - h) / 2;
            size_t p2stride = p2w * sizeof(uint32_t);

            int32_t b_off_x, b_off_y, a_off_x, a_off_y;

            if(off_x < 0)
            {
                b_off_x = 0;
                a_off_x = -off_x;
            } else {
                b_off_x = off_x;
                a_off_x = 0;
            }

            if(off_y < 0)
            {
                b_off_y = 0;
                a_off_y = -off_y;
            } else {
                b_off_y = off_y;
                a_off_y = 0;
            }

            uint8_t* b_ptr0 = ba + b_off_y * p2stride + b_off_x*4;

            int32_t nw = w - a_off_x*2;
            int32_t nh = h - a_off_y*2;

            #if defined (ANE_MSW) // we need flip Y

                const uint8_t* a_ptr0 = (const uint8_t*)(frame + a_off_y * widthInBytes + a_off_x*3);
                const uint8_t* a_ptr = a_ptr0;
                const uint8_t* a_ptr2 = a_ptr0 + (nh - 1) * widthInBytes;
                uint8_t* b_ptr = b_ptr0 + (h - 1) * p2stride;
                uint8_t* b_ptr2 = b_ptr0;
                const uint8_t alpha = 0xFF;
                for(i = 0; i < nh / 2; i++)
                {
                    uint8_t *ba_ln = b_ptr;
                    uint8_t *ba_ln2 = b_ptr2;
                    const uint8_t *a_ln = a_ptr;
                    const uint8_t *a_ln2 = a_ptr2;
                    for(j = 0; j < nw; j++, ba_ln += 4, ba_ln2 += 4, a_ln += 3, a_ln2 += 3)
                    {

                        *ba_ln = *(a_ln+0);
                        ba_ln[1] = *(a_ln+1);
                        ba_ln[2] = *(a_ln+2);
                        ba_ln[3] = alpha;
                        //
                        *ba_ln2 = *(a_ln2+0);
                        ba_ln2[1] = *(a_ln2+1);
                        ba_ln2[2] = *(a_ln2+2);
                        ba_ln2[3] = alpha;
                    }
                    
                    a_ptr += widthInBytes;
                    a_ptr2 -= widthInBytes;
                    b_ptr -= p2stride;
                    b_ptr2 += p2stride;
                }
                if(nh&1)
                {
                    for(j = 0; j < nw; j++, b_ptr += 4, a_ptr+=3)
                    {
                        *b_ptr = *(a_ptr+0);
                        b_ptr[1] = *(a_ptr+1);
                        b_ptr[2] = *(a_ptr+2);
                        b_ptr[3] = alpha;
                    }
                }

            #elif defined(ANE_MAC) || defined (ANE_IOS) || defined (__ANDROID__) 

                const uint8_t* a_ptr0 = (const uint8_t*)(frame + a_off_y * widthInBytes + a_off_x*4);

                for(i = 0; i < nh; i++)
                {
                    memcpy(b_ptr0, a_ptr0, nw*4);
                    a_ptr0 += widthInBytes;
                    b_ptr0 += p2stride;
                }

            #endif
            
            FREReleaseByteArray(objectBA);
        }

        FRENewObjectFromInt32(1, &res_obj);
    } 
    else if(cap)
    {
        // lets see if device is available it may be freezed
        if( captureCheckResponse(cap) == 0 )
        {
            FRENewObjectFromInt32(-1, &res_obj);
        }
    }

    return res_obj;
}

FREObject supportsSaveToCameraRoll(FREContext ctx, void* funcData, uint32_t argc, FREObject argv[])
{
    FREObject res_obj;

    int32_t res = captureSupportsSaveToCameraRoll();
    FRENewObjectFromInt32(res, &res_obj);
    
    return res_obj;
}

FREObject saveToCameraRoll(FREContext ctx, void* funcData, uint32_t argc, FREObject argv[])
{
    uint32_t name_size = 0;
    const uint8_t* name_val = NULL;
    
    FREGetObjectAsUTF8(argv[0], &name_size, &name_val);
    
    FREObject objectBA = argv[1];
    FREByteArray baData;
    FREAcquireByteArray(objectBA, &baData);
    uint8_t *ba = baData.bytes;
    
    int32_t _size;
    FREGetObjectAsInt32(argv[2], &_size);
    
    int32_t res = captureSaveToCameraRoll( (const char *)name_val, (const uint8_t*)ba, _size );
    
    FREReleaseByteArray(objectBA);
    
    FREObject res_obj;
    FRENewObjectFromInt32(res, &res_obj);
    
    return res_obj;
}

FREObject focusAtPoint(FREContext ctx, void* funcData, uint32_t argc, FREObject argv[])
{
    int32_t _id;
    double _x, _y;
    FREGetObjectAsInt32(argv[0], &_id);
    
    FREGetObjectAsDouble(argv[1], &_x);
    FREGetObjectAsDouble(argv[2], &_y);
    
    CCapture* cap;
    cap = active_cams[_id];
    
    if(cap)
    {
        captureFocusAtPoint(cap, (float)_x, (float)_y);
    }
    return NULL;
}

void stillImageCallback(const uint8_t *data, int32_t size)
{
    if(_cam_shot_data) free(_cam_shot_data);
    _cam_shot_data = (uint8_t*)malloc(size);
    
    memcpy(_cam_shot_data, data, size);
    
    _cam_shot_size = size;
    FREDispatchStatusEventAsync( 
                                _ctx, 
                                (const uint8_t*)"CAM_SHOT", 
                                (const uint8_t*)"0");
    
}

FREObject grabCamShot(FREContext ctx, void* funcData, uint32_t argc, FREObject argv[])
{
    if(_cam_shot_data)
    {
        FREObject length = NULL;
        FRENewObjectFromUint32(_cam_shot_size, &length);
        FRESetObjectProperty(argv[0], (const uint8_t*) "length", length, NULL);
    
        FREObject objectBA = argv[0];
        FREByteArray baData;
        FREAcquireByteArray(objectBA, &baData);
        uint8_t *ba = baData.bytes;
        
        memcpy(ba, _cam_shot_data, _cam_shot_size);
    
        FREReleaseByteArray(objectBA);
        
        free(_cam_shot_data);
        _cam_shot_size = 0;
        _cam_shot_data = NULL;
    }
    
    return NULL;
}

FREObject captureStillImage(FREContext ctx, void* funcData, uint32_t argc, FREObject argv[])
{
    int32_t _id;
    FREGetObjectAsInt32(argv[0], &_id);
    
    CCapture* cap;
    cap = active_cams[_id];
    
    if(cap)
    {
        captureGetStillImage(cap, stillImageCallback);
    }
    
    return NULL;
}

FREObject disposeANE(FREContext ctx, void* funcData, uint32_t argc, FREObject argv[])
{
    dispose_ane();
    return NULL;
}

//
// default init routines
//
void contextInitializer(void* extData, const uint8_t* ctxType, FREContext ctx, uint32_t* numFunctions, const FRENamedFunction** functions)
{
    _ctx = ctx;
    
    *numFunctions = 11;

    FRENamedFunction* func = (FRENamedFunction*) malloc(sizeof(FRENamedFunction) * (*numFunctions));

    func[0].name = (const uint8_t*) "listDevices";
    func[0].functionData = NULL;
    func[0].function = &listDevices;

    func[1].name = (const uint8_t*) "getCapture";
    func[1].functionData = NULL;
    func[1].function = &getCapture;

    func[2].name = (const uint8_t*) "releaseCapture";
    func[2].functionData = NULL;
    func[2].function = &delCapture;

    func[3].name = (const uint8_t*) "getCaptureFrame";
    func[3].functionData = NULL;
    func[3].function = &getCaptureFrame;

    func[4].name = (const uint8_t*) "toggleCapturing";
    func[4].functionData = NULL;
    func[4].function = &toggleCapturing;
    
    func[5].name = (const uint8_t*) "supportsSaveToCameraRoll";
    func[5].functionData = NULL;
    func[5].function = &supportsSaveToCameraRoll;
    
    func[6].name = (const uint8_t*) "saveToCameraRoll";
    func[6].functionData = NULL;
    func[6].function = &saveToCameraRoll;
    
    func[7].name = (const uint8_t*) "focusAtPoint";
    func[7].functionData = NULL;
    func[7].function = &focusAtPoint;
    
    func[8].name = (const uint8_t*) "camShot";
    func[8].functionData = NULL;
    func[8].function = &captureStillImage;
    
    func[9].name = (const uint8_t*) "grabCamShot";
    func[9].functionData = NULL;
    func[9].function = &grabCamShot;

    func[10].name = (const uint8_t*) "disposeANE";
    func[10].functionData = NULL;
    func[10].function = &disposeANE;

    *functions = func;

    memset(active_cams, 0, sizeof(CCapture*) * MAX_ACTIVE_CAMS);
}

void contextFinalizer(FREContext ctx)
{
    dispose_ane();
}

void captureInitializer(void** extData, FREContextInitializer* ctxInitializer, FREContextFinalizer* ctxFinalizer)
{
    *ctxInitializer = &contextInitializer;
    *ctxFinalizer = &contextFinalizer;
}

void captureFinalizer(void* extData)
{
    return;
}
