
#include "yuv_convert.h"
#include "yuv_to_rgb_table.h"
#include "convert_yuv_to_rgb.h"

#define packuswb(x) ((x) < 0 ? 0 : ((x) > 255 ? 255 : (x)))
#define paddsw(x, y) (((x) + (y)) < -32768 ? -32768 : \
    (((x) + (y)) > 32767 ? 32767 : ((x) + (y))))

static __inline__ void ConvertYUVToRGB32_C(uint8_t y,
                                       uint8_t u,
                                       uint8_t v,
                                       uint8_t* rgb_buf) {
  int b = kCoefficientsRgbY[256+u][0];
  int g = kCoefficientsRgbY[256+u][1];
  int r = kCoefficientsRgbY[256+u][2];
  int a = kCoefficientsRgbY[256+u][3];

  b = paddsw(b, kCoefficientsRgbY[512+v][0]);
  g = paddsw(g, kCoefficientsRgbY[512+v][1]);
  r = paddsw(r, kCoefficientsRgbY[512+v][2]);
  a = paddsw(a, kCoefficientsRgbY[512+v][3]);

  b = paddsw(b, kCoefficientsRgbY[y][0]);
  g = paddsw(g, kCoefficientsRgbY[y][1]);
  r = paddsw(r, kCoefficientsRgbY[y][2]);
  a = paddsw(a, kCoefficientsRgbY[y][3]);

  b >>= 6;
  g >>= 6;
  r >>= 6;
  a >>= 6;

  *reinterpret_cast<uint32_t*>(rgb_buf) = (packuswb(b)) |
                                        (packuswb(g) << 8) |
                                        (packuswb(r) << 16) |
                                        (packuswb(a) << 24);
}

// 16.16 fixed point arithmetic
const int kFractionBits = 16;
const int kFractionMax = 1 << kFractionBits;
const int kFractionMask = ((1 << kFractionBits) - 1);

extern "C" {

void ConvertYUVToRGB32Row_C(const uint8_t* y_buf,
                            const uint8_t* u_buf,
                            const uint8_t* v_buf,
                            uint8_t* rgb_buf,
                            int width) {
  for (int x = 0; x < width; x += 2) {
    uint8_t u = u_buf[x >> 1];
    uint8_t v = v_buf[x >> 1];
    uint8_t y0 = y_buf[x];
    ConvertYUVToRGB32_C(y0, u, v, rgb_buf);
    if ((x + 1) < width) {
      uint8_t y1 = y_buf[x + 1];
      ConvertYUVToRGB32_C(y1, u, v, rgb_buf + 4);
    }
    rgb_buf += 8;  // Advance 2 pixels.
  }
}

void ConvertYUVToRGB32_C(const uint8_t* yplane,
                         const uint8_t* uplane,
                         const uint8_t* vplane,
                         uint8_t* rgbframe,
                         int width,
                         int height,
                         int ystride,
                         int uvstride,
                         int rgbstride,
                         YUVType yuv_type) {
  unsigned int y_shift = yuv_type;
  for (int y = 0; y < height; ++y) {
    uint8_t* rgb_row = rgbframe + y * rgbstride;
    const uint8_t* y_ptr = yplane + y * ystride;
    const uint8_t* u_ptr = uplane + (y >> y_shift) * uvstride;
    const uint8_t* v_ptr = vplane + (y >> y_shift) * uvstride;

    ConvertYUVToRGB32Row_C(y_ptr,
                           u_ptr,
                           v_ptr,
                           rgb_row,
                           width);
  }
}

} // extern C