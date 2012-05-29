#ifndef CCAPTURE_CONFIG_H_
#define CCAPTURE_CONFIG_H_


///////////////// C DEFINITIONS /////////////////////

#if defined WIN32 || defined _WIN32
    #define C_CDECL __cdecl
    #define C_STDCALL __stdcall
#else
    #define C_CDECL
    #define C_STDCALL
#endif

#ifndef C_EXTERN_C
    #ifdef __cplusplus
        #define C_EXTERN_C extern "C"
        #define C_DEFAULT(val) = val
    #else
        #define C_EXTERN_C
        #define C_DEFAULT(val)
    #endif
#endif

    
#if (defined WIN32 || defined _WIN32 || defined WINCE) && defined API_EXPORTS
    #define C_EXPORTS __declspec(dllexport)
#else
    #define C_EXPORTS
#endif

#ifndef CAPI
    #define CAPI(rettype) C_EXTERN_C C_EXPORTS rettype C_CDECL
#endif

#ifndef C_IMPL
#define C_IMPL C_EXTERN_C
#endif

#endif
