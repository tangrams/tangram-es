# Tell Android where to locate source files
# "my-dir" is a macro function which will return the path of the current directory (where Android.mk resides)
LOCAL_PATH := $(call my-dir)

# prepare libtess2
include $(CLEAR_VARS)
LOCAL_MODULE    := libtess2
LOCAL_SRC_FILES := precompiled/$(TARGET_ARCH_ABI)/libtess2.a
include $(PREBUILT_STATIC_LIBRARY)

# Clear contents of the LOCAL_* variables
include $(CLEAR_VARS)

# prepare libcurl
# prebuilt binaries from https://github.com/minggo/libcurl-build/tree/master/prebuild-with-ssl
include $(CLEAR_VARS)
LOCAL_MODULE    := libcurl
LOCAL_SRC_FILES := precompiled/$(TARGET_ARCH_ABI)/libcurl.a
include $(PREBUILT_STATIC_LIBRARY)

# Clear contents of the LOCAL_* variables
include $(CLEAR_VARS)

# All the source files to include in this module
LOCAL_SRC_FILES := ../../core/src/tangram.cpp jniExports.cpp platform_android.cpp
LOCAL_SRC_FILES += ../../core/include/jsoncpp/jsoncpp.cpp
LOCAL_SRC_FILES += $(wildcard $(LOCAL_PATH)/../../core/src/util/*.cpp)
LOCAL_SRC_FILES += $(wildcard $(LOCAL_PATH)/../../core/src/dataSource/*.cpp)
LOCAL_SRC_FILES += $(wildcard $(LOCAL_PATH)/../../core/src/mapTile/*.cpp)
LOCAL_SRC_FILES += $(wildcard $(LOCAL_PATH)/../../core/src/sceneDefinition/*.cpp)
LOCAL_SRC_FILES += $(wildcard $(LOCAL_PATH)/../../core/src/sceneDirector/*.cpp)
LOCAL_SRC_FILES += $(wildcard $(LOCAL_PATH)/../../core/src/style/*.cpp)
LOCAL_SRC_FILES += $(wildcard $(LOCAL_PATH)/../../core/src/tileManager/*.cpp)
LOCAL_SRC_FILES += $(wildcard $(LOCAL_PATH)/../../core/src/viewModule/*.cpp)

# The name of the module
LOCAL_MODULE := tangram

# Compilation flags
LOCAL_CFLAGS := -Werror -DPLATFORM_ANDROID -fexceptions

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include-all include ../../core/src/ ../../core/include/ ../../core/include/jsoncpp

# Static libraries to link with
LOCAL_LDLIBS := -llog -lGLESv2 -lz
LOCAL_STATIC_LIBRARIES := libcurl libtess2 

# Build the module
include $(BUILD_SHARED_LIBRARY)