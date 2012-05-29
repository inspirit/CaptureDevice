#ifndef CONVERT_H_
#define CONVERT_H_

#include "CaptureConfig.h"
#include "CCaptureConfig.h"

/* Constants for color conversion */

#define BGR2BGRA      0
#define RGB2RGBA      BGR2BGRA
#define BGRA2BGR      1
#define RGBA2RGB      BGRA2BGR
#define BGR2RGBA      2
#define RGB2BGRA      BGR2RGBA
#define RGBA2BGR      3
#define BGRA2RGB      RGBA2BGR
#define BGR2RGB       4
#define RGB2BGR       BGR2RGB
#define BGRA2RGBA     5
#define RGBA2BGRA     BGRA2RGBA
#define BGR2GRAY      6
#define RGB2GRAY      7
#define GRAY2BGR      8
#define GRAY2RGB      GRAY2BGR
#define GRAY2BGRA     9
#define GRAY2RGBA     GRAY2BGRA
#define BGRA2GRAY     10
#define RGBA2GRAY     11
#define YUV2RGB_NV12  90
#define YUV2BGR_NV12  91    
#define YUV2RGB_NV21  92
#define YUV2BGR_NV21  93
#define YUV420sp2RGB YUV2RGB_NV21
#define YUV420sp2BGR YUV2BGR_NV21
#define YUV2RGBA_NV12  94
#define YUV2BGRA_NV12  95
#define YUV2RGBA_NV21  96
#define YUV2BGRA_NV21  97
#define YUV420sp2RGBA YUV2RGBA_NV21
#define YUV420sp2BGRA YUV2BGRA_NV21
#define YUV2RGB_YV12  98
#define YUV2BGR_YV12  99
#define YUV2RGB_IYUV  100
#define YUV2BGR_IYUV  101
#define YUV2RGB_I420 YUV2RGB_IYUV
#define YUV2BGR_I420 YUV2BGR_IYUV
#define YUV420p2RGB YUV2RGB_YV12
#define YUV420p2BGR YUV2BGR_YV12
#define YUV2RGBA_YV12  102
#define YUV2BGRA_YV12  103
#define YUV2RGBA_IYUV  104
#define YUV2BGRA_IYUV  105
#define YUV2RGBA_I420  YUV2RGBA_IYUV
#define YUV2BGRA_I420  YUV2BGRA_IYUV
#define YUV420p2RGBA  YUV2RGBA_YV12
#define YUV420p2BGRA  YUV2BGRA_YV12
#define YUV2GRAY_420  106
#define YUV2GRAY_NV21  YUV2GRAY_420
#define YUV2GRAY_NV12  YUV2GRAY_420
#define YUV2GRAY_YV12  YUV2GRAY_420
#define YUV2GRAY_IYUV  YUV2GRAY_420
#define YUV2GRAY_I420  YUV2GRAY_420
#define YUV420sp2GRAY  YUV2GRAY_420
#define YUV420p2GRAY  YUV2GRAY_420

#ifdef __cplusplus

    template<typename _Tp> static inline _Tp saturate_cast(uint8_t v) { return _Tp(v); }
    template<typename _Tp> static inline _Tp saturate_cast(signed char v) { return _Tp(v); }
    template<typename _Tp> static inline _Tp saturate_cast(unsigned short v) { return _Tp(v); }
    template<typename _Tp> static inline _Tp saturate_cast(short v) { return _Tp(v); }
    template<typename _Tp> static inline _Tp saturate_cast(unsigned v) { return _Tp(v); }
    template<typename _Tp> static inline _Tp saturate_cast(int v) { return _Tp(v); }
    template<typename _Tp> static inline _Tp saturate_cast(float v) { return _Tp(v); }
    template<typename _Tp> static inline _Tp saturate_cast(double v) { return _Tp(v); }

    template<> inline uint8_t saturate_cast<uint8_t>(int8_t v)
    { return (uint8_t)std::max((int)v, 0); }
    template<> inline uint8_t saturate_cast<uint8_t>(unsigned short v)
    { return (uint8_t)std::min((unsigned)v, (unsigned)UCHAR_MAX); }
    template<> inline uint8_t saturate_cast<uint8_t>(int v)
    { return (uint8_t)((unsigned)v <= UCHAR_MAX ? v : v > 0 ? UCHAR_MAX : 0); }
    template<> inline uint8_t saturate_cast<uint8_t>(short v)
    { return saturate_cast<uint8_t>((int)v); }
    template<> inline uint8_t saturate_cast<uint8_t>(unsigned v)
    { return (uint8_t)std::min(v, (unsigned)UCHAR_MAX); }
    template<> inline uint8_t saturate_cast<uint8_t>(float v)
    { int iv = lrintf(v); return saturate_cast<uint8_t>(iv); }

    void convertColor(const uint8_t* src_ptr, uint8_t* dst_ptr, 
                            int width, int height, int src_ch, int dst_ch, int code, int depth);

#endif
/*
#ifdef __cplusplus
extern "C" {
#endif

    CAPI(void) convertColor(const uint8_t* src_ptr, uint8_t* dst_ptr, int width, int height, int code);

#ifdef __cplusplus
}
#endif
*/
#endif