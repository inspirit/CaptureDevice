LOCAL_PATH := $(call my-dir)

### COPY FLASH RUNTIME ###

include $(CLEAR_VARS)
LOCAL_MODULE := FlashRuntime
LOCAL_SRC_FILES := FlashRuntimeExtensions.so
include $(PREBUILT_SHARED_LIBRARY)

### android camera support lib ###

include $(CLEAR_VARS)
LOCAL_MODULE := opencv_androidcamera
LOCAL_SRC_FILES := camera_activity.cpp
LOCAL_LDLIBS +=  -llog -ldl 
LOCAL_CPPFLAGS += -frtti -fexceptions -O2 
LOCAL_SHARED_LIBRARIES += liblog
include $(BUILD_STATIC_LIBRARY)

########################################

include $(CLEAR_VARS)

MY_SRC_PATH := ../../src

LOCAL_MODULE := capture
LOCAL_SRC_FILES :=  $(MY_SRC_PATH)/CaptureANE.c \
                    $(MY_SRC_PATH)/Capture.cpp \
                    $(MY_SRC_PATH)/CaptureSurface.cpp \
                    $(MY_SRC_PATH)/CCapture.cpp \
                    $(MY_SRC_PATH)/ColorConvert.cpp \
                    $(MY_SRC_PATH)/impl/CaptureImplAndroid.cpp 

LOCAL_LDLIBS +=  -llog -ldl 
LOCAL_CPPFLAGS += -frtti -fexceptions -O3 
LOCAL_SHARED_LIBRARIES += FlashRuntime liblog
LOCAL_STATIC_LIBRARIES += opencv_androidcamera 

LOCAL_CFLAGS := -O3
include $(BUILD_SHARED_LIBRARY)

