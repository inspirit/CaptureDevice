#ifndef CAPTURE_SURFACE_H_
#define CAPTURE_SURFACE_H_

#include "CaptureConfig.h"

template<typename T>
class CaptureSurfaceT 
{
 private:
    struct Obj {
        Obj( int32_t aWidth, int32_t aHeight, T *aData, bool aOwnsData, int32_t aRowBytes, int32_t aNumChannels );
        ~Obj();
        void        setDeallocator( void(*aDeallocatorFunc)( void * ), void *aDeallocatorRefcon );

        int32_t                        mWidth, mHeight, mRowBytes, mNumChannels;
        T                            *mData;
        bool                        mOwnsData;

        void                        (*mDeallocatorFunc)(void *refcon);
        void                        *mDeallocatorRefcon;
    };

 public:

    CaptureSurfaceT() {}
    
    CaptureSurfaceT( int32_t width, int32_t height, int32_t channels );
    CaptureSurfaceT( T *data, int32_t width, int32_t height, int32_t rowBytes );

    int32_t            getWidth() const { return mObj->mWidth; }
    int32_t            getHeight() const { return mObj->mHeight; }
    int32_t            getRowBytes() const { return mObj->mRowBytes; }
    
    int32_t            getNumChannels() const { return mObj->mNumChannels; }
    void             flipX();
    void             flipY();
    void             transposeInplace();

    T*                    getData() { return mObj->mData; }
    const T*            getData() const { return mObj->mData; }

    void                setDeallocator( void(*aDeallocatorFunc)( void * ), void *aDeallocatorRefcon );

    typedef std::shared_ptr<Obj> CaptureSurfaceT::*unspecified_bool_type;
    operator unspecified_bool_type() const { return ( mObj.get() == 0 ) ? 0 : &CaptureSurfaceT::mObj; }
    void reset() { mObj.reset(); }

private:
    std::shared_ptr<Obj>        mObj;

 };

 class CaptureSurfaceExc : public std::exception {
    virtual const char* what() const throw() {
        return "CaptureSurface exception";
    }
};

typedef CaptureSurfaceT<uint8_t> CaptureSurface;
typedef CaptureSurfaceT<uint8_t> CaptureSurface8u;

#endif
