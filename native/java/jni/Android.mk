LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

MY_SRC_PATH := ../../src

LOCAL_MODULE := color_convert
LOCAL_SRC_FILES :=  jni_convert.cpp \
                    yuv2rgb_neon.c.arm.neon \
                    memcpy.S.arm.neon \
                    nv21_rgb.S.arm.neon \
                    nv12_rgb.S.arm.neon \
                    i420_rgb.S.arm.neon \
                    $(MY_SRC_PATH)/ColorConvert.cpp 

LOCAL_LDLIBS +=  -llog 
LOCAL_CPPFLAGS += -frtti -fexceptions -O3 
LOCAL_SHARED_LIBRARIES += liblog
LOCAL_STATIC_LIBRARIES := cpufeatures

LOCAL_CFLAGS := -O3
include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/cpufeatures)

#convert_yuv_to_rgb.cpp \
#                    yuv_convert.cpp \
#                    yuv_to_rgb_table.cpp \