
#include <android/log.h>
#include <jni.h>
#include <math.h>

#if !defined(LOGD) && !defined(LOGI) && !defined(LOGE)
#define LOG_TAG "CAPTURE"
#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))
#endif

#include <stdio.h>
#include <sys/types.h>
#include <cstddef>
#include <new>
#include <cstdlib>
#include <cpu-features.h>

using namespace std;

#include "../../src/ColorConvert.h"

// A wrapper class to ensure that GetByteArrayElements is always properly
// paired with a ReleaseByteArrayElements.
class jniByteArray
{
private:
    // Purposely not implemented to prevent copying.
    jniByteArray(const jniByteArray &);
    jniByteArray & operator=(const jniByteArray &);

    JNIEnv * m_env;
    jbyteArray m_byteArray;
    jbyte * m_bytes;
    jboolean m_isCopy;
public:
    jniByteArray(JNIEnv * env, jbyteArray bytes) : m_env(env), m_byteArray(bytes), m_bytes(env->GetByteArrayElements(bytes, &m_isCopy))
    {
        if (!m_bytes)
        throw std::bad_alloc();
    }

    ~jniByteArray()
    {
        m_env->ReleaseByteArrayElements(m_byteArray, m_bytes, 0);
    }

    uint8_t * getBytes()
    {
        return reinterpret_cast<uint8_t *>(m_bytes);
    }

    std::size_t getSize() const
    {
        return std::size_t(m_env->GetArrayLength(m_byteArray));
    }

    bool isCopy() const
    {
        return bool(m_isCopy) ;
    }
};

#define MALLOC_ALIGN (16)

static __inline__ void* alignPtr(void* ptr, int n)
{
    return (void*)(((size_t)ptr + n-1) & -n);
}

static __inline__ void* cvAlloc( size_t size)
{
    uint8_t* udata = (uint8_t*)malloc(size + sizeof(void*) + MALLOC_ALIGN);
    if(!udata) return 0;
    uint8_t** adata = (uint8_t**)alignPtr((uint8_t**)udata + 1, MALLOC_ALIGN);
    adata[-1] = udata;
    return adata;
}

static __inline__ void cvFree( void* ptr )
{
    if(ptr)
    {
        uint8_t* udata = ((uint8_t**)ptr)[-1];
        free(udata);
    }
}

extern "C" void nv21_2_rgba_neon(uint8_t *src_ptr, uint8_t *dst_ptr, int width, int height);
extern "C" void yv12_2_rgba_neon(uint8_t *src_ptr, uint8_t *dst_ptr, int width, int height);
extern "C" void * memcpy_neon(void * destination, const void * source, size_t size);

//#include "convert_yuv_to_rgb.h"

static uint8_t* input_buff = 0;
static uint8_t* output_buff = 0;

extern "C" JNIEXPORT void JNICALL Java_ru_inspirit_capture_CameraSurface_setupConvert( 
                                                    JNIEnv * env, jclass, 
                                                    jint width, jint height
                                                    )
{
    if(input_buff) cvFree(input_buff);
    input_buff = (uint8_t*)cvAlloc(width*height*3/2);

    if(output_buff) cvFree(output_buff);
    output_buff = (uint8_t*)cvAlloc(width*height*4);
}

extern "C" JNIEXPORT void JNICALL Java_ru_inspirit_capture_CameraSurface_disposeConvert(JNIEnv * env, jclass)
{
    if(input_buff) cvFree(input_buff);
    if(output_buff) cvFree(output_buff);
}
//
static __inline__ size_t align16(size_t sz)
{
    return ((sz) + 15) & ~15;
}
//
extern "C" JNIEXPORT void JNICALL Java_ru_inspirit_capture_CameraSurface_convert( 
                                                    JNIEnv * env,
                                                    jclass, 
                                                    jbyteArray input, 
                                                    jbyteArray output, 
                                                    jint width,
                                                    jint height, 
                                                    jint format )
{
    static int32_t CPU_HAVE_NEON = (int32_t)(android_getCpuFamily() == ANDROID_CPU_FAMILY_ARM 
                                    && (android_getCpuFeatures() & ANDROID_CPU_ARM_FEATURE_NEON) != 0);

    const bool inRGBorder = false;
    const bool withAlpha = true;
    const int clrDepth = 8;

    jniByteArray _jinput(env, input);
    jniByteArray _joutput(env, output);

    if (format == 0)
    {
        if(CPU_HAVE_NEON)
        {
            memcpy_neon(input_buff, _jinput.getBytes(), width*height*3/2);

            nv21_2_rgba_neon(input_buff, output_buff, width, height);

            memcpy_neon(_joutput.getBytes(), output_buff, width*height*4);
        }
        else
        {
            convertColor(_jinput.getBytes(), _joutput.getBytes(), 
                        width, height*3/2,
                        1, withAlpha ? 4 : 3,
                        inRGBorder ? YUV420sp2RGB : YUV420sp2BGR, clrDepth);
        }
    } 
    else if (format == 1)
    {
        if(CPU_HAVE_NEON)
        {
            memcpy_neon(input_buff, _jinput.getBytes(), width*height*3/2);

            yv12_2_rgba_neon(input_buff, output_buff, width, height);

            memcpy_neon(_joutput.getBytes(), output_buff, width*height*4);
        }
       /* else
        {
            const size_t stride = align16(width);
            const size_t y_size = stride * height;
            const size_t uv_size = align16(stride/2) * height/2;
            const uint8_t* y = (const uint8_t*)_jinput.getBytes();

            const uint8_t *u = y + y_size;
            const uint8_t *v = u + uv_size;

            ConvertYUVToRGB32_C(y,
                             u,
                             v,
                             _joutput.getBytes(),
                             width,
                             height,
                             stride,
                             align16(stride/2),
                             width<<2,
                             YV12);
        }*/
    }
}
