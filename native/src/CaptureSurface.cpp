
#include "CaptureSurface.h"

//
#include <string.h>
#ifdef _WIN32
#include <malloc.h>
#else
#include <alloca.h>
#endif
//

template<typename T>
CaptureSurfaceT<T>::Obj::Obj( int32_t aWidth, int32_t aHeight, 
                                T *aData, bool aOwnsData, int32_t aRowBytes, int32_t aNumChannels )
{
    mWidth = aWidth;
    mHeight = aHeight;
    mData = aData;
    mOwnsData = aOwnsData;
    mRowBytes = aRowBytes;
    mNumChannels = aNumChannels;
    mDeallocatorFunc = NULL;
}

template<typename T>
CaptureSurfaceT<T>::Obj::~Obj()
{
    if( mDeallocatorFunc )
        (*mDeallocatorFunc)( mDeallocatorRefcon );

    if( mOwnsData )
        delete [] mData;
}

template<typename T>
void CaptureSurfaceT<T>::Obj::setDeallocator( void(*aDeallocatorFunc)( void * ), void *aDeallocatorRefcon )
{
    mDeallocatorFunc = aDeallocatorFunc;
    mDeallocatorRefcon = aDeallocatorRefcon;
}

template<typename T>
CaptureSurfaceT<T>::CaptureSurfaceT( int32_t aWidth, int32_t aHeight, int32_t numChannels )
{
    int32_t rowBytes = aWidth * sizeof(T) * numChannels;
    T *data = new T[aHeight * rowBytes]; // TODO is it correct? it should be new T[aHeight * aWidth * numChannels]
    mObj = std::shared_ptr<Obj>( new Obj( aWidth, aHeight, data, true, rowBytes, numChannels ) );
}

template<typename T>
CaptureSurfaceT<T>::CaptureSurfaceT( T *aData, int32_t aWidth, int32_t aHeight, int32_t aRowBytes )
{
    int32_t numChannels = aRowBytes / aWidth;
    mObj = std::shared_ptr<Obj>( new Obj( aWidth, aHeight, aData, false, aRowBytes, numChannels ) );
}

template<typename T>
void CaptureSurfaceT<T>::setDeallocator( void(*aDeallocatorFunc)( void * ), void *aDeallocatorRefcon )
{
    mObj->setDeallocator( aDeallocatorFunc, aDeallocatorRefcon );
}

template<typename T>
void CaptureSurfaceT<T>::flipX()
{
    int32_t i, j;
    int32_t len = sizeof(T) * mObj->mNumChannels;
    int32_t step = mObj->mRowBytes;
    unsigned char* buffer = (unsigned char*)alloca(len);
    unsigned char* a_ptr = (unsigned char*)mObj->mData;
    int32_t rows = mObj->mHeight;
    int32_t cols = mObj->mWidth;
    for (i = 0; i < rows; i++)
    {
        for (j = 0; j < cols / 2; j++)
        {
            memcpy(buffer, a_ptr + j * len, len);
            memcpy(a_ptr + j * len, a_ptr + (cols - 1 - j) * len, len);
            memcpy(a_ptr + (cols - 1 - j) * len, buffer, len);
        }
        a_ptr += step;
    }
}

template<typename T>
void CaptureSurfaceT<T>::flipY()
{
    int32_t i;
    int32_t step = mObj->mRowBytes;
    int32_t rows = mObj->mHeight;
    //int32_t cols = mObj->mWidth;
    unsigned char* buffer = (unsigned char*)alloca(step);
    unsigned char* a_ptr = (unsigned char*)mObj->mData;
    unsigned char* b_ptr = a_ptr + (rows - 1) * step;
    for (i = 0; i < rows / 2; i++)
    {
        memcpy(buffer, a_ptr, step);
        memcpy(a_ptr, b_ptr, step);
        memcpy(b_ptr, buffer, step);
        a_ptr += step;
        b_ptr -= step;
    }
}

template<typename T>
void CaptureSurfaceT<T>::transposeInplace()
{
    int32_t start, next, i;
    T *m = mObj->mData;
    T tmp;
    int32_t w = mObj->mWidth;
    int32_t h = mObj->mHeight;
 
    for (start = 0; start <= w * h - 1; start++) {
        next = start;
        i = 0;
        do {    i++;
            next = (next % h) * w + next / h;
        } while (next > start);
        if (next < start || i == 1) continue;
 
        tmp = m[next = start];
        do {
            i = (next % h) * w + next / h;
            m[next] = (i == start) ? tmp : m[i];
            next = i;
        } while (next > start);
    }
}

template class CaptureSurfaceT<uint8_t>;
