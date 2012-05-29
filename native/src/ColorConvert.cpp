
#include <limits>
#include <math.h>
#include "ColorConvert.h"

using namespace std;

template<typename _Tp> struct ColorChannel
{
    typedef float worktype_f;
    static _Tp max() { return std::numeric_limits<_Tp>::max(); }
    static _Tp half() { return (_Tp)(max()/2 + 1); } 
};

template<> struct ColorChannel<float>
{
    typedef float worktype_f;
    static float max() { return 1.f; }
    static float half() { return 0.5f; }
};

///////////////////////////// Top-level template function ////////////////////////////////

template<class Ct> void CtColorLoop(const uint8_t* src_ptr, uint8_t* dst_ptr, 
                                    int width, int height, int src_ch, int dst_ch, const Ct& cvt)
{
    typedef typename Ct::channel_type _Tp;
    size_t srcstep = width*src_ch, dststep = width*dst_ch;
    
    //if( srcmat.isContinuous() && dstmat.isContinuous() )
    {
        width *= height;
        height = 1;
    }    
    
    for( ; height--; src_ptr += srcstep, dst_ptr += dststep )
        cvt((const _Tp*)src_ptr, (_Tp*)dst_ptr, width);
}
    
    
////////////////// Various 3/4-channel to 3/4-channel RGB transformations /////////////////
    
template<typename _Tp> struct RGB2RGB
{
    typedef _Tp channel_type;
    
    RGB2RGB(int _srccn, int _dstcn, int _blueIdx) : srccn(_srccn), dstcn(_dstcn), blueIdx(_blueIdx) {}
    void operator()(const _Tp* src, _Tp* dst, int n) const
    {
        int scn = srccn, dcn = dstcn, bidx = blueIdx;
        if( dcn == 3 )
        {
            n *= 3;
            for( int i = 0; i < n; i += 3, src += scn )
            {
                _Tp t0 = src[bidx], t1 = src[1], t2 = src[bidx ^ 2];
                dst[i] = t0; dst[i+1] = t1; dst[i+2] = t2;
            }
        }
        else if( scn == 3 )
        {
            n *= 3;
            _Tp alpha = ColorChannel<_Tp>::max();
            for( int i = 0; i < n; i += 3, dst += 4 )
            {
                _Tp t0 = src[i], t1 = src[i+1], t2 = src[i+2];
                dst[bidx] = t0; dst[1] = t1; dst[bidx^2] = t2; dst[3] = alpha;
            }
        }
        else
        {
            n *= 4;
            for( int i = 0; i < n; i += 4 )
            {
                _Tp t0 = src[i], t1 = src[i+1], t2 = src[i+2], t3 = src[i+3];
                dst[i] = t2; dst[i+1] = t1; dst[i+2] = t0; dst[i+3] = t3;
            }
        }
    }
    
    int srccn, dstcn, blueIdx;
};



///////////////////////////////////// YUV420 -> RGB /////////////////////////////////////

const int ITUR_BT_601_CY = 1220542;
const int ITUR_BT_601_CUB = 2116026;
const int ITUR_BT_601_CUG = -409993;
const int ITUR_BT_601_CVG = -852492;
const int ITUR_BT_601_CVR = 1673527;
const int ITUR_BT_601_SHIFT = 20;

template<int bIdx, int uIdx>
struct YUV420sp2RGB888Invoker
{
    uint8_t* dst;
    const uint8_t* my1, *muv;
    int width, stride;

    YUV420sp2RGB888Invoker(uint8_t* _dst, int _width, int _stride, const uint8_t* _y1, const uint8_t* _uv)
        : dst(_dst), my1(_y1), muv(_uv), width(_width), stride(_stride) {}

    void operator()(int begin, int end) const
    {
        int rangeBegin = begin * 2;
        int rangeEnd = end * 2;

        //R = 1.164(Y - 16) + 1.596(V - 128)
        //G = 1.164(Y - 16) - 0.813(V - 128) - 0.391(U - 128)
        //B = 1.164(Y - 16)                  + 2.018(U - 128)

        //R = (1220542(Y - 16) + 1673527(V - 128)                  + (1 << 19)) >> 20
        //G = (1220542(Y - 16) - 852492(V - 128) - 409993(U - 128) + (1 << 19)) >> 20
        //B = (1220542(Y - 16)                  + 2116026(U - 128) + (1 << 19)) >> 20

        const uint8_t* y1 = my1 + rangeBegin * stride, *uv = muv + rangeBegin * stride / 2;

        for (int j = rangeBegin; j < rangeEnd; j += 2, y1 += stride * 2, uv += stride)
        {
            uint8_t* row1 = dst + j * (width*3);//dst->ptr<uint8_t>(j);
            uint8_t* row2 = row1 + (width*3);//dst->ptr<uint8_t>(j + 1);
            const uint8_t* y2 = y1 + stride;

            for (int i = 0; i < width; i += 2, row1 += 6, row2 += 6)
            {
                int u = int(uv[i + 0 + uIdx]) - 128;
                int v = int(uv[i + 1 - uIdx]) - 128;

                int ruv = (1 << (ITUR_BT_601_SHIFT - 1)) + ITUR_BT_601_CVR * v;
                int guv = (1 << (ITUR_BT_601_SHIFT - 1)) + ITUR_BT_601_CVG * v + ITUR_BT_601_CUG * u;
                int buv = (1 << (ITUR_BT_601_SHIFT - 1)) + ITUR_BT_601_CUB * u;

                int y00 = std::max(0, int(y1[i]) - 16) * ITUR_BT_601_CY;
                row1[2-bIdx] = saturate_cast<uint8_t>((y00 + ruv) >> ITUR_BT_601_SHIFT);
                row1[1]      = saturate_cast<uint8_t>((y00 + guv) >> ITUR_BT_601_SHIFT);
                row1[bIdx]   = saturate_cast<uint8_t>((y00 + buv) >> ITUR_BT_601_SHIFT);

                int y01 = std::max(0, int(y1[i + 1]) - 16) * ITUR_BT_601_CY;
                row1[5-bIdx] = saturate_cast<uint8_t>((y01 + ruv) >> ITUR_BT_601_SHIFT);
                row1[4]      = saturate_cast<uint8_t>((y01 + guv) >> ITUR_BT_601_SHIFT);
                row1[3+bIdx] = saturate_cast<uint8_t>((y01 + buv) >> ITUR_BT_601_SHIFT);

                int y10 = std::max(0, int(y2[i]) - 16) * ITUR_BT_601_CY;
                row2[2-bIdx] = saturate_cast<uint8_t>((y10 + ruv) >> ITUR_BT_601_SHIFT);
                row2[1]      = saturate_cast<uint8_t>((y10 + guv) >> ITUR_BT_601_SHIFT);
                row2[bIdx]   = saturate_cast<uint8_t>((y10 + buv) >> ITUR_BT_601_SHIFT);

                int y11 = std::max(0, int(y2[i + 1]) - 16) * ITUR_BT_601_CY;
                row2[5-bIdx] = saturate_cast<uint8_t>((y11 + ruv) >> ITUR_BT_601_SHIFT);
                row2[4]      = saturate_cast<uint8_t>((y11 + guv) >> ITUR_BT_601_SHIFT);
                row2[3+bIdx] = saturate_cast<uint8_t>((y11 + buv) >> ITUR_BT_601_SHIFT);
            }
        }
    }
};

template<int bIdx, int uIdx>
struct YUV420sp2RGBA8888Invoker
{
    uint8_t* dst;
    const uint8_t* my1, *muv;
    int width, stride;

    YUV420sp2RGBA8888Invoker(uint8_t* _dst, int _width, int _stride, const uint8_t* _y1, const uint8_t* _uv)
        : dst(_dst), my1(_y1), muv(_uv), width(_width), stride(_stride) {}

    void operator()(int begin, int end) const
    {
        int rangeBegin = begin * 2;
        int rangeEnd = end * 2;

        //R = 1.164(Y - 16) + 1.596(V - 128)
        //G = 1.164(Y - 16) - 0.813(V - 128) - 0.391(U - 128)
        //B = 1.164(Y - 16)                  + 2.018(U - 128)

        //R = (1220542(Y - 16) + 1673527(V - 128)                  + (1 << 19)) >> 20
        //G = (1220542(Y - 16) - 852492(V - 128) - 409993(U - 128) + (1 << 19)) >> 20
        //B = (1220542(Y - 16)                  + 2116026(U - 128) + (1 << 19)) >> 20

        const uint8_t* y1 = my1 + rangeBegin * stride, *uv = muv + rangeBegin * stride / 2;

        for (int j = rangeBegin; j < rangeEnd; j += 2, y1 += stride * 2, uv += stride)
        {
            uint8_t* row1 = dst + j * (width*4);//dst->ptr<uint8_t>(j);
            uint8_t* row2 = row1 + (width*4);//dst->ptr<uint8_t>(j + 1);
            const uint8_t* y2 = y1 + stride;

            for (int i = 0; i < width; i += 2, row1 += 8, row2 += 8)
            {
                int u = int(uv[i + 0 + uIdx]) - 128;
                int v = int(uv[i + 1 - uIdx]) - 128;

                int ruv = (1 << (ITUR_BT_601_SHIFT - 1)) + ITUR_BT_601_CVR * v;
                int guv = (1 << (ITUR_BT_601_SHIFT - 1)) + ITUR_BT_601_CVG * v + ITUR_BT_601_CUG * u;
                int buv = (1 << (ITUR_BT_601_SHIFT - 1)) + ITUR_BT_601_CUB * u;

                int y00 = std::max(0, int(y1[i]) - 16) * ITUR_BT_601_CY;
                row1[2-bIdx] = saturate_cast<uint8_t>((y00 + ruv) >> ITUR_BT_601_SHIFT);
                row1[1]      = saturate_cast<uint8_t>((y00 + guv) >> ITUR_BT_601_SHIFT);
                row1[bIdx]   = saturate_cast<uint8_t>((y00 + buv) >> ITUR_BT_601_SHIFT);
                row1[3]      = uint8_t(0xff);

                int y01 = std::max(0, int(y1[i + 1]) - 16) * ITUR_BT_601_CY;
                row1[6-bIdx] = saturate_cast<uint8_t>((y01 + ruv) >> ITUR_BT_601_SHIFT);
                row1[5]      = saturate_cast<uint8_t>((y01 + guv) >> ITUR_BT_601_SHIFT);
                row1[4+bIdx] = saturate_cast<uint8_t>((y01 + buv) >> ITUR_BT_601_SHIFT);
                row1[7]      = uint8_t(0xff);

                int y10 = std::max(0, int(y2[i]) - 16) * ITUR_BT_601_CY;
                row2[2-bIdx] = saturate_cast<uint8_t>((y10 + ruv) >> ITUR_BT_601_SHIFT);
                row2[1]      = saturate_cast<uint8_t>((y10 + guv) >> ITUR_BT_601_SHIFT);
                row2[bIdx]   = saturate_cast<uint8_t>((y10 + buv) >> ITUR_BT_601_SHIFT);
                row2[3]      = uint8_t(0xff);

                int y11 = std::max(0, int(y2[i + 1]) - 16) * ITUR_BT_601_CY;
                row2[6-bIdx] = saturate_cast<uint8_t>((y11 + ruv) >> ITUR_BT_601_SHIFT);
                row2[5]      = saturate_cast<uint8_t>((y11 + guv) >> ITUR_BT_601_SHIFT);
                row2[4+bIdx] = saturate_cast<uint8_t>((y11 + buv) >> ITUR_BT_601_SHIFT);
                row2[7]      = uint8_t(0xff);
            }
        }
    }
};

template<int bIdx>
struct YUV420p2RGB888Invoker
{
    uint8_t* dst;
    const uint8_t* my1, *mu, *mv;
    int width, stride;
    int ustepIdx, vstepIdx;

    YUV420p2RGB888Invoker(uint8_t* _dst, int _width, int _stride, const uint8_t* _y1, const uint8_t* _u, const uint8_t* _v, int _ustepIdx, int _vstepIdx)
        : dst(_dst), my1(_y1), mu(_u), mv(_v), width(_width), stride(_stride), ustepIdx(_ustepIdx), vstepIdx(_vstepIdx) {}

    void operator()(int begin, int end) const
    {
        const int rangeBegin = begin * 2;
        const int rangeEnd = end * 2;
        
        size_t uvsteps[2] = {width/2, stride - width/2};
        int usIdx = ustepIdx, vsIdx = vstepIdx;

        const uint8_t* y1 = my1 + rangeBegin * stride;
        const uint8_t* u1 = mu + (begin / 2) * stride;
        const uint8_t* v1 = mv + (begin / 2) * stride;
        
        if(begin % 2 == 1)
        {
            u1 += uvsteps[(usIdx++) & 1];
            v1 += uvsteps[(vsIdx++) & 1];
        }

        for (int j = rangeBegin; j < rangeEnd; j += 2, y1 += stride * 2, u1 += uvsteps[(usIdx++) & 1], v1 += uvsteps[(vsIdx++) & 1])
        {
            uint8_t* row1 = dst + j * (width*3);//dst->ptr<uint8_t>(j);
            uint8_t* row2 = row1 + (width*3);//dst->ptr<uint8_t>(j + 1);
            const uint8_t* y2 = y1 + stride;

            for (int i = 0; i < width / 2; i += 1, row1 += 6, row2 += 6)
            {
                int u = int(u1[i]) - 128;
                int v = int(v1[i]) - 128;

                int ruv = (1 << (ITUR_BT_601_SHIFT - 1)) + ITUR_BT_601_CVR * v;
                int guv = (1 << (ITUR_BT_601_SHIFT - 1)) + ITUR_BT_601_CVG * v + ITUR_BT_601_CUG * u;
                int buv = (1 << (ITUR_BT_601_SHIFT - 1)) + ITUR_BT_601_CUB * u;

                int y00 = std::max(0, int(y1[2 * i]) - 16) * ITUR_BT_601_CY;
                row1[2-bIdx] = saturate_cast<uint8_t>((y00 + ruv) >> ITUR_BT_601_SHIFT);
                row1[1]      = saturate_cast<uint8_t>((y00 + guv) >> ITUR_BT_601_SHIFT);
                row1[bIdx]   = saturate_cast<uint8_t>((y00 + buv) >> ITUR_BT_601_SHIFT);

                int y01 = std::max(0, int(y1[2 * i + 1]) - 16) * ITUR_BT_601_CY;
                row1[5-bIdx] = saturate_cast<uint8_t>((y01 + ruv) >> ITUR_BT_601_SHIFT);
                row1[4]      = saturate_cast<uint8_t>((y01 + guv) >> ITUR_BT_601_SHIFT);
                row1[3+bIdx] = saturate_cast<uint8_t>((y01 + buv) >> ITUR_BT_601_SHIFT);

                int y10 = std::max(0, int(y2[2 * i]) - 16) * ITUR_BT_601_CY;
                row2[2-bIdx] = saturate_cast<uint8_t>((y10 + ruv) >> ITUR_BT_601_SHIFT);
                row2[1]      = saturate_cast<uint8_t>((y10 + guv) >> ITUR_BT_601_SHIFT);
                row2[bIdx]   = saturate_cast<uint8_t>((y10 + buv) >> ITUR_BT_601_SHIFT);

                int y11 = std::max(0, int(y2[2 * i + 1]) - 16) * ITUR_BT_601_CY;
                row2[5-bIdx] = saturate_cast<uint8_t>((y11 + ruv) >> ITUR_BT_601_SHIFT);
                row2[4]      = saturate_cast<uint8_t>((y11 + guv) >> ITUR_BT_601_SHIFT);
                row2[3+bIdx] = saturate_cast<uint8_t>((y11 + buv) >> ITUR_BT_601_SHIFT);
            }
        }
    }
};

template<int bIdx>
struct YUV420p2RGBA8888Invoker
{
    uint8_t* dst;
    const uint8_t* my1, *mu, *mv;
    int width, stride;
    int ustepIdx, vstepIdx;

    YUV420p2RGBA8888Invoker(uint8_t* _dst, int _width, int _stride, const uint8_t* _y1, const uint8_t* _u, const uint8_t* _v, int _ustepIdx, int _vstepIdx)
        : dst(_dst), my1(_y1), mu(_u), mv(_v), width(_width), stride(_stride), ustepIdx(_ustepIdx), vstepIdx(_vstepIdx) {}

    void operator()(int begin, int end) const
    {
        int rangeBegin = begin * 2;
        int rangeEnd = end * 2;

        size_t uvsteps[2] = {width/2, stride - width/2};
        int usIdx = ustepIdx, vsIdx = vstepIdx;

        const uint8_t* y1 = my1 + rangeBegin * stride;
        const uint8_t* u1 = mu + (begin / 2) * stride;
        const uint8_t* v1 = mv + (begin / 2) * stride;
        
        if(begin % 2 == 1)
        {
            u1 += uvsteps[(usIdx++) & 1];
            v1 += uvsteps[(vsIdx++) & 1];
        }

        for (int j = rangeBegin; j < rangeEnd; j += 2, y1 += stride * 2, u1 += uvsteps[(usIdx++) & 1], v1 += uvsteps[(vsIdx++) & 1])
        {
            uint8_t* row1 = dst + j * (width*4);//dst->ptr<uint8_t>(j);
            uint8_t* row2 = row1 + (width*4);//dst->ptr<uint8_t>(j + 1);
            const uint8_t* y2 = y1 + stride;

            for (int i = 0; i < width / 2; i += 1, row1 += 8, row2 += 8)
            {
                int u = int(u1[i]) - 128;
                int v = int(v1[i]) - 128;

                int ruv = (1 << (ITUR_BT_601_SHIFT - 1)) + ITUR_BT_601_CVR * v;
                int guv = (1 << (ITUR_BT_601_SHIFT - 1)) + ITUR_BT_601_CVG * v + ITUR_BT_601_CUG * u;
                int buv = (1 << (ITUR_BT_601_SHIFT - 1)) + ITUR_BT_601_CUB * u;

                int y00 = std::max(0, int(y1[2 * i]) - 16) * ITUR_BT_601_CY;
                row1[2-bIdx] = saturate_cast<uint8_t>((y00 + ruv) >> ITUR_BT_601_SHIFT);
                row1[1]      = saturate_cast<uint8_t>((y00 + guv) >> ITUR_BT_601_SHIFT);
                row1[bIdx]   = saturate_cast<uint8_t>((y00 + buv) >> ITUR_BT_601_SHIFT);
                row1[3]      = uint8_t(0xff);

                int y01 = std::max(0, int(y1[2 * i + 1]) - 16) * ITUR_BT_601_CY;
                row1[6-bIdx] = saturate_cast<uint8_t>((y01 + ruv) >> ITUR_BT_601_SHIFT);
                row1[5]      = saturate_cast<uint8_t>((y01 + guv) >> ITUR_BT_601_SHIFT);
                row1[4+bIdx] = saturate_cast<uint8_t>((y01 + buv) >> ITUR_BT_601_SHIFT);
                row1[7]      = uint8_t(0xff);

                int y10 = std::max(0, int(y2[2 * i]) - 16) * ITUR_BT_601_CY;
                row2[2-bIdx] = saturate_cast<uint8_t>((y10 + ruv) >> ITUR_BT_601_SHIFT);
                row2[1]      = saturate_cast<uint8_t>((y10 + guv) >> ITUR_BT_601_SHIFT);
                row2[bIdx]   = saturate_cast<uint8_t>((y10 + buv) >> ITUR_BT_601_SHIFT);
                row2[3]      = uint8_t(0xff);

                int y11 = std::max(0, int(y2[2 * i + 1]) - 16) * ITUR_BT_601_CY;
                row2[6-bIdx] = saturate_cast<uint8_t>((y11 + ruv) >> ITUR_BT_601_SHIFT);
                row2[5]      = saturate_cast<uint8_t>((y11 + guv) >> ITUR_BT_601_SHIFT);
                row2[4+bIdx] = saturate_cast<uint8_t>((y11 + buv) >> ITUR_BT_601_SHIFT);
                row2[7]      = uint8_t(0xff);
            }
        }
    }
};

#define MIN_SIZE_FOR_PARALLEL_YUV420_CONVERSION (320*240)

template<int bIdx, int uIdx>
inline void cvtYUV420sp2RGB(uint8_t* _dst, int _width, int _height, int _stride, const uint8_t* _y1, const uint8_t* _uv)
{
    YUV420sp2RGB888Invoker<bIdx, uIdx> converter(_dst, _width, _stride, _y1,  _uv);
#ifdef HAVE_TBB
    if (_width*_height >= MIN_SIZE_FOR_PARALLEL_YUV420_CONVERSION)
        parallel_for(0, _height/2, converter);
    else
#endif
        converter(0, _height/2);
}

template<int bIdx, int uIdx>
inline void cvtYUV420sp2RGBA(uint8_t* _dst, int _width, int _height, int _stride, const uint8_t* _y1, const uint8_t* _uv)
{
    YUV420sp2RGBA8888Invoker<bIdx, uIdx> converter(_dst, _width, _stride, _y1,  _uv);
#ifdef HAVE_TBB
    if (_width*_height >= MIN_SIZE_FOR_PARALLEL_YUV420_CONVERSION)
        parallel_for(0, _height/2, converter);
    else
#endif
        converter(0, _height/2);
}

template<int bIdx>
inline void cvtYUV420p2RGB(uint8_t* _dst, int _width, int _height, int _stride, const uint8_t* _y1, const uint8_t* _u, const uint8_t* _v, int ustepIdx, int vstepIdx)
{
    YUV420p2RGB888Invoker<bIdx> converter(_dst, _width, _stride, _y1,  _u, _v, ustepIdx, vstepIdx);
#ifdef HAVE_TBB
    if (_width*_height >= MIN_SIZE_FOR_PARALLEL_YUV420_CONVERSION)
        parallel_for(0, _height/2, converter);
    else
#endif
        converter(0, _height/2);
}

template<int bIdx>
inline void cvtYUV420p2RGBA(uint8_t* _dst, int _width, int _height, int _stride, const uint8_t* _y1, const uint8_t* _u, const uint8_t* _v, int ustepIdx, int vstepIdx)
{
    YUV420p2RGBA8888Invoker<bIdx> converter(_dst, _width, _stride, _y1,  _u, _v, ustepIdx, vstepIdx);
#ifdef HAVE_TBB
    if (_width*_height >= MIN_SIZE_FOR_PARALLEL_YUV420_CONVERSION)
        parallel_for(0, _height/2, converter);
    else
#endif
        converter(0, _height/2);
}

//////////////////////////////////////////////
/////////////////////////////////////////////

/*C_IMPL */void convertColor(const uint8_t* src_ptr, uint8_t* dst_ptr, 
                            int width, int height, int src_ch, int dst_ch, int code, int depth)
{
    //int depth = 8;
    int bidx;

    switch( code )
    {
        case BGR2BGRA: case RGB2BGRA: case BGRA2BGR:
        case RGBA2BGR: case RGB2BGR: case BGRA2RGBA:
            {
                if (dst_ch <= 0) dst_ch = code == BGR2BGRA || code == RGB2BGRA || code == BGRA2RGBA ? 4 : 3;
                const int bidx = code == BGR2BGRA || code == BGRA2BGR ? 0 : 2;
                
                if( depth == 8 )
                {
                    CtColorLoop(src_ptr, dst_ptr, width, height, src_ch, dst_ch, RGB2RGB<uint8_t>(src_ch, dst_ch, bidx));
                }
                else if( depth == 16 )
                {
                    CtColorLoop(src_ptr, dst_ptr, width, height, src_ch, dst_ch, RGB2RGB<unsigned short>(src_ch, dst_ch, bidx));
                } else {
                    CtColorLoop(src_ptr, dst_ptr, width, height, src_ch, dst_ch, RGB2RGB<float>(src_ch, dst_ch, bidx));
                }
            }
            break;
        case YUV2BGR_NV21:  case YUV2RGB_NV21:  case YUV2BGR_NV12:  case YUV2RGB_NV12:
        case YUV2BGRA_NV21: case YUV2RGBA_NV21: case YUV2BGRA_NV12: case YUV2RGBA_NV12:
            {
                // http://www.fourcc.org/yuv.php#NV21 == yuv420sp -> a plane of 8 bit Y samples followed by an interleaved V/U plane containing 8 bit 2x2 subsampled chroma samples
                // http://www.fourcc.org/yuv.php#NV12 -> a plane of 8 bit Y samples followed by an interleaved U/V plane containing 8 bit 2x2 subsampled colour difference samples
                
                if (dst_ch <= 0) dst_ch = (code==YUV420sp2BGRA || code==YUV420sp2RGBA || code==YUV2BGRA_NV12 || code==YUV2RGBA_NV12) ? 4 : 3;
                const int bidx = (code==YUV2BGR_NV21 || code==YUV2BGRA_NV21 || code==YUV2BGR_NV12 || code==YUV2BGRA_NV12) ? 0 : 2;
                const int uidx = (code==YUV2BGR_NV21 || code==YUV2BGRA_NV21 || code==YUV2RGB_NV21 || code==YUV2RGBA_NV21) ? 1 : 0;
                
                
                int dst_w = width;
                int dst_h = height * 2 / 3;

                int srcstep = width;
                const uint8_t* y = src_ptr;
                const uint8_t* uv = y + srcstep * dst_h;
                
                switch(dst_ch*100 + bidx * 10 + uidx)
                {
                    case 300: cvtYUV420sp2RGB<0, 0> (dst_ptr, dst_w, dst_h, srcstep, y, uv); break;
                    case 301: cvtYUV420sp2RGB<0, 1> (dst_ptr, dst_w, dst_h, srcstep, y, uv); break;
                    case 320: cvtYUV420sp2RGB<2, 0> (dst_ptr, dst_w, dst_h, srcstep, y, uv); break;
                    case 321: cvtYUV420sp2RGB<2, 1> (dst_ptr, dst_w, dst_h, srcstep, y, uv); break;
                    case 400: cvtYUV420sp2RGBA<0, 0>(dst_ptr, dst_w, dst_h, srcstep, y, uv); break;
                    case 401: cvtYUV420sp2RGBA<0, 1>(dst_ptr, dst_w, dst_h, srcstep, y, uv); break;
                    case 420: cvtYUV420sp2RGBA<2, 0>(dst_ptr, dst_w, dst_h, srcstep, y, uv); break;
                    case 421: cvtYUV420sp2RGBA<2, 1>(dst_ptr, dst_w, dst_h, srcstep, y, uv); break;
                    //default: CV_Error( CV_StsBadFlag, "Unknown/unsupported color conversion code" ); break;
                };
            }
            break;
        case YUV2BGR_YV12: case YUV2RGB_YV12: case YUV2BGRA_YV12: case YUV2RGBA_YV12:
        case YUV2BGR_IYUV: case YUV2RGB_IYUV: case YUV2BGRA_IYUV: case YUV2RGBA_IYUV:
            {
                //http://www.fourcc.org/yuv.php#YV12 == yuv420p -> It comprises an NxM Y plane followed by (N/2)x(M/2) V and U planes.
                //http://www.fourcc.org/yuv.php#IYUV == I420 -> It comprises an NxN Y plane followed by (N/2)x(N/2) U and V planes
                
                if (dst_ch <= 0) dst_ch = (code==YUV2BGRA_YV12 || code==YUV2RGBA_YV12 || code==YUV2RGBA_IYUV || code==YUV2BGRA_IYUV) ? 4 : 3;
                const int bidx = (code==YUV2BGR_YV12 || code==YUV2BGRA_YV12 || code==YUV2BGR_IYUV || code==YUV2BGRA_IYUV) ? 0 : 2;
                const int uidx = (code==YUV2BGR_YV12 || code==YUV2RGB_YV12 || code==YUV2BGRA_YV12 || code==YUV2RGBA_YV12) ? 1 : 0;
                
                //CV_Assert( dcn == 3 || dcn == 4 );
                //CV_Assert( sz.width % 2 == 0 && sz.height % 3 == 0 && depth == CV_8U );

                //Size dstSz(sz.width, sz.height * 2 / 3);
                //_dst.create(dstSz, CV_MAKETYPE(depth, dst_ch));
                //dst = _dst.getMat();
                int dst_w = width;
                int dst_h = height * 2 / 3;
                
                int srcstep = width;
                const uint8_t* y = src_ptr;
                const uint8_t* u = y + srcstep * dst_h;
                const uint8_t* v = y + srcstep * (dst_h + dst_h/4) + (dst_w/2) * ((dst_h % 4)/2);
                
                int ustepIdx = 0;
                int vstepIdx = dst_h % 4 == 2 ? 1 : 0;
                
                if(uidx == 1) { std::swap(u ,v), std::swap(ustepIdx, vstepIdx); };
                
                switch(dst_ch*10 + bidx)
                {
                    case 30: cvtYUV420p2RGB<0>(dst_ptr, dst_w, dst_h, srcstep, y, u, v, ustepIdx, vstepIdx); break;
                    case 32: cvtYUV420p2RGB<2>(dst_ptr, dst_w, dst_h, srcstep, y, u, v, ustepIdx, vstepIdx); break;
                    case 40: cvtYUV420p2RGBA<0>(dst_ptr, dst_w, dst_h, srcstep, y, u, v, ustepIdx, vstepIdx); break;
                    case 42: cvtYUV420p2RGBA<2>(dst_ptr, dst_w, dst_h, srcstep, y, u, v, ustepIdx, vstepIdx); break;
                    //default: CV_Error( CV_StsBadFlag, "Unknown/unsupported color conversion code" ); break;
                };
            }
            break;
        case YUV2GRAY_420:
            {
                if (dst_ch <= 0) dst_ch = 1;
                
                //CV_Assert( dst_ch == 1 );
                //CV_Assert( sz.width % 2 == 0 && sz.height % 3 == 0 && depth == CV_8U );

                //Size dstSz(sz.width, sz.height * 2 / 3);
                //_dst.create(dstSz, CV_MAKETYPE(depth, dst_ch));
                //dst = _dst.getMat();
                
                //src(Range(0, dstSz.height), Range::all()).copyTo(dst);
            }
            break;
    }
}