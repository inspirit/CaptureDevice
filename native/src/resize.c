#include <alloca.h>
#include "resize.h"

/* area interpolation resample is adopted from OpenCV */

#define ccv_clamp(x, a, b) (((x) < (a)) ? (a) : (((x) > (b)) ? (b) : (x)))
#define ccv_min(a, b) (((a) < (b)) ? (a) : (b))
#define ccv_max(a, b) (((a) > (b)) ? (a) : (b))

typedef struct {
    int si, di;
    unsigned int alpha;
} ccv_int_alpha;

void resample_area_8u(const uint8_t* a, int32_t src_width, int32_t src_height, 
                        uint8_t* b, int32_t dst_width, int32_t dst_height,
                        int32_t ch)
{
    ccv_int_alpha* xofs = (ccv_int_alpha*)alloca(sizeof(ccv_int_alpha) * src_width * 2);
    double scale_x = (double)src_width / dst_width;
    double scale_y = (double)src_height / dst_height;
    unsigned int inv_scale_256 = (int)(scale_x * scale_y * 0x10000);
    int dx, dy, sx, sy, i, k;
    int src_step = src_width * ch;
    int dst_step = dst_width * ch;
    for (dx = 0, k = 0; dx < dst_width; dx++)
    {
        double fsx1 = dx * scale_x, fsx2 = fsx1 + scale_x;
        int sx1 = (int)(fsx1 + 1.0 - 1e-6), sx2 = (int)(fsx2);
        sx1 = ccv_min(sx1, src_width - 1);
        sx2 = ccv_min(sx2, src_width - 1);

        if (sx1 > fsx1)
        {
            xofs[k].di = dx * ch;
            xofs[k].si = (sx1 - 1) * ch;
            xofs[k++].alpha = (unsigned int)((sx1 - fsx1) * 0x100);
        }

        for (sx = sx1; sx < sx2; sx++)
        {
            xofs[k].di = dx * ch;
            xofs[k].si = sx * ch;
            xofs[k++].alpha = 256;
        }

        if (fsx2 - sx2 > 1e-3)
        {
            xofs[k].di = dx * ch;
            xofs[k].si = sx2 * ch;
            xofs[k++].alpha = (unsigned int)((fsx2 - sx2) * 256);
        }
    }
    int xofs_count = k;
    unsigned int* buf = (unsigned int*)alloca(dst_width * ch * sizeof(unsigned int));
    unsigned int* sum = (unsigned int*)alloca(dst_width * ch * sizeof(unsigned int));
    for (dx = 0; dx < dst_width * ch; dx++)
        buf[dx] = sum[dx] = 0;
    dy = 0;
    for (sy = 0; sy < src_height; sy++)
    {
        const uint8_t* a_ptr = a + src_step * sy;
        for (k = 0; k < xofs_count; k++)
        {
            int dxn = xofs[k].di;
            unsigned int alpha = xofs[k].alpha;
            for (i = 0; i < ch; i++)
                buf[dxn + i] += a_ptr[xofs[k].si + i] * alpha;
        }
        if ((dy + 1) * scale_y <= sy + 1 || sy == src_height - 1)
        {
            unsigned int beta = (int)(ccv_max(sy + 1 - (dy + 1) * scale_y, 0.f) * 256);
            unsigned int beta1 = 256 - beta;
            unsigned char* b_ptr = b + dst_step * dy;
            if (beta <= 0)
            {
                for (dx = 0; dx < dst_width * ch; dx++)
                {
                    b_ptr[dx] = ccv_clamp((sum[dx] + buf[dx] * 256) / inv_scale_256, 0, 255);
                    sum[dx] = buf[dx] = 0;
                }
            } else {
                for (dx = 0; dx < dst_width * ch; dx++)
                {
                    b_ptr[dx] = ccv_clamp((sum[dx] + buf[dx] * beta1) / inv_scale_256, 0, 255);
                    sum[dx] = buf[dx] * beta;
                    buf[dx] = 0;
                }
            }
            dy++;
        }
        else
        {
            for(dx = 0; dx < dst_width * ch; dx++)
            {
                sum[dx] += buf[dx] * 256;
                buf[dx] = 0;
            }
        }
    }
}
