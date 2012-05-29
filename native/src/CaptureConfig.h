#ifndef CAPTURE_CONFIG_H_
#define CAPTURE_CONFIG_H_


#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
    #define _ENV_MSW_
#elif defined(linux) || defined(__linux) || defined(__linux__)
    #define _ENV_LINUX_
#elif defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)
    #define _ENV_COCOA_
    #include "TargetConditionals.h"
    #if TARGET_OS_IPHONE
        #define _ENV_COCOA_TOUCH_
    #else
        #define _ENV_MAC_
    #endif
    // This is defined to prevent the inclusion of some unfortunate macros in <AssertMacros.h>
    #define __ASSERTMACROS__
#elif defined (__ANDROID__)
    #define _ENV_ANDROID_
#else
    #error "compile error: Unknown platform"
#endif

#ifndef _MSC_VER
    #include <unistd.h>
    #include <stdint.h>
#endif

#ifdef __MINGW32__
    #include <stdint.h>
#else
    #ifdef WIN32
        typedef unsigned __int32    uint32_t;
        typedef unsigned __int8     uint8_t;
        typedef          __int32    int32_t;
    #else
        #include <stdint.h>
    #endif
#endif

#if defined( _MSC_VER ) && ( _MSC_VER >= 1600 )
    #include <memory>
#elif defined( _ENV_COCOA_ )
    #include <tr1/memory>
    namespace std {
        using std::tr1::shared_ptr;
        using std::tr1::weak_ptr;        
        using std::tr1::static_pointer_cast;
        using std::tr1::dynamic_pointer_cast;
        using std::tr1::const_pointer_cast;
        using std::tr1::enable_shared_from_this;
    }
#else
    #include <tr1/memory>
    namespace std {
        using std::tr1::shared_ptr;
        using std::tr1::weak_ptr;        
        using std::tr1::static_pointer_cast;
        using std::tr1::dynamic_pointer_cast;
        using std::tr1::const_pointer_cast;
        using std::tr1::enable_shared_from_this;
    }
#endif

#endif
    