#ifndef _YUV_CONVERT_H_
#define _YUV_CONVERT_H_

typedef unsigned int   uint32_t;
typedef signed   int   int32_t;
typedef unsigned short uint16_t;
typedef unsigned char  uint8_t;

// Type of YUV surface.
// The value of these enums matter as they are used to shift vertical indices.
enum YUVType {
  YV16 = 0,           // YV16 is half width and full height chroma channels.
  YV12 = 1,           // YV12 is half width and half height chroma channels.
};

#endif