
#include <arm_neon.h>


struct yuv_planes
{
    void *y, *u, *v;
    size_t pitch;
};

/* Packed picture buffer. Pitch is in bytes (_not_ pixels). */
struct yuv_pack
{
    void *yuv;
    size_t pitch;
};

/* I420 to RGBA conversion. */
void i420_rgb_neon (struct yuv_pack *const out,
                   const struct yuv_planes *const in,
                   int width, int height) asm("i420_rgb_neon");

/* NV21 to RGBA conversion. */
void nv21_rgb_neon (struct yuv_pack *const out,
                   const struct yuv_planes *const in,
                   int width, int height) asm("nv21_rgb_neon");

/* NV12 to RGBA conversion. */
void nv12_rgb_neon (struct yuv_pack *const out,
                   const struct yuv_planes *const in,
                   int width, int height) asm("nv12_rgb_neon");

void nv21_2_rgba_neon(uint8_t *src_ptr, uint8_t *dst_ptr, int width, int height)
{
    const size_t y_size = width * height;
    const size_t uv_size = y_size >> 2;
    uint8_t* y = src_ptr;
    uint8_t* uv = src_ptr + y_size;

    
    struct yuv_pack out = { dst_ptr, width*4 };
    struct yuv_planes in = { y, uv, uv, width };
    nv12_rgb_neon (&out, &in, width, height);
}

static __inline__ size_t align16(size_t sz)
{
    return ((sz) + 15) & ~15;
}

void yv12_2_rgba_neon(uint8_t *src_ptr, uint8_t *dst_ptr, int width, int height)
{
    const size_t stride = align16(width);
    const size_t y_size = stride * height;
    const size_t uv_size = align16(stride/2) * height/2;
    uint8_t* y = src_ptr;

    uint8_t *u = y + y_size;
    uint8_t *v = u + uv_size;

    struct yuv_pack out = { dst_ptr, width*4 };
    struct yuv_planes in = { y, v, u, stride };
    i420_rgb_neon (&out, &in, width, height);
}

