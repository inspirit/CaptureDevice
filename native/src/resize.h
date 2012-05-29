#ifndef RESIZE_H
#define RESIZE_H

#include "CaptureConfig.h"

void resample_area_8u(const uint8_t* a, int32_t src_width, int32_t src_height, 
                        uint8_t* b, int32_t dst_width, int32_t dst_height,
                        int32_t ch);

#endif