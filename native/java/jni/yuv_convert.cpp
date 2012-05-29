#include "yuv_convert.h"
#include "convert_yuv_to_rgb.h"
#include <cstring>

void ConvertYUY2ToYUV(const uint8_t* src,
                      uint8_t* yplane,
                      uint8_t* uplane,
                      uint8_t* vplane,
                      int width,
                      int height) {
  for (int i = 0; i < height / 2; ++i) {
    for (int j = 0; j < (width / 2); ++j) {
      yplane[0] = src[0];
      *uplane = src[1];
      yplane[1] = src[2];
      *vplane = src[3];
      src += 4;
      yplane += 2;
      uplane++;
      vplane++;
    }
    for (int j = 0; j < (width / 2); ++j) {
      yplane[0] = src[0];
      yplane[1] = src[2];
      src += 4;
      yplane += 2;
    }
  }
}

void ConvertNV21ToYUV(const uint8_t* src,
                      uint8_t* yplane,
                      uint8_t* uplane,
                      uint8_t* vplane,
                      int width,
                      int height) {
  int y_plane_size = width * height;
  memcpy(yplane, src, y_plane_size);

  src += y_plane_size;
  int u_plane_size = y_plane_size >> 2;
  for (int i = 0; i < u_plane_size; ++i) {
    *vplane++ = *src++;
    *uplane++ = *src++;
  }
}

void ConvertYUVToRGB32(const uint8_t* yplane,
                       const uint8_t* uplane,
                       const uint8_t* vplane,
                       uint8_t* rgbframe,
                       int width,
                       int height,
                       int ystride,
                       int uvstride,
                       int rgbstride,
                       YUVType yuv_type) {

  ConvertYUVToRGB32_C(yplane, uplane, vplane, rgbframe,
                      width, height, ystride, uvstride, rgbstride, yuv_type);
}