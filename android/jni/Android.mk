# Tell Android where to locate source files
# "my-dir" is a macro function which will return the path of the current directory (where Android.mk resides)
LOCAL_PATH := $(call my-dir)

# Clear contents of the LOCAL_* variables
include $(CLEAR_VARS)

# All the source files to include in this module
LOCAL_SRC_FILES := ../../core/tangram.cpp ../../core/util/ShaderProgram.cpp jniExports.cpp platform_android.cpp

# The name of the module
LOCAL_MODULE := tangram

# Compilation flags
LOCAL_CFLAGS := -Werror -DPLATFORM_ANDROID

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include-all ../../core/

# Static libraries to link with
LOCAL_LDLIBS := -llog -lGLESv2

# Build the module
include $(BUILD_SHARED_LIBRARY)