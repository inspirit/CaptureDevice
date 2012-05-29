#ifndef _CONVERT_YUV_TO_RGB_H_
#define _CONVERT_YUV_TO_RGB_H_

#include "yuv_convert.h"

extern "C" {

void ConvertYUVToRGB32Row_C(const uint8_t* yplane,
                            const uint8_t* uplane,
                            const uint8_t* vplane,
                            uint8_t* rgbframe,
                            int width);

void ConvertYUVToRGB32_C(const uint8_t* yplane,
                         const uint8_t* uplane,
                         const uint8_t* vplane,
                         uint8_t* rgbframe,
                         int width,
                         int height,
                         int ystride,
                         int uvstride,
                         int rgbstride,
                         YUVType yuv_type);

}

#endif