include(${CMAKE_SOURCE_DIR}/build/toolchains/android.toolchain.cmake)

add_definitions(-DPLATFORM_ANDROID)

if(NOT MAKE_BUILD_TOOL)
	message(SEND_ERROR "Provide ndk make, located in <NDK_ROOT>/prebuilt/<OS>/bin/, -DMAKE_BUILD_TOOL=<NDK_PREBUILT_BIN>/make")
	return()
else()
	set(CMAKE_BUILD_TOOL ${MAKE_BUILD_TOOL})
	message(STATUS "Will use make prebuilt tool located at : ${CMAKE_BUILD_TOOL}")
endif()

set(ANDROID_ARCHITECTURE "armeabi-v7a")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11 -pedantic -llog -lz")
set(CXX_FLAGS_DEBUG "${CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -pedantic -fPIC")
set(EXECUTABLE_NAME "tangram.out")
set(ANDROID_TARGET "android-19")

set(CORE_LIB_TYPE SHARED)
set(CORE_LIB_DEPS GLESv2 ${CMAKE_SOURCE_DIR}/android/jni/precompiled/${ANDROID_ARCHITECTURE}/libcurl.a)

include_directories(${PROJECT_SOURCE_DIR}/android/jni/include)

set(ADDITIONNAL_TARGET_DEPENDENT_SRC_FILES 
     ${CMAKE_SOURCE_DIR}/android/jni/jniExports.cpp
     ${CMAKE_SOURCE_DIR}/android/jni/platform_android.cpp)

# load core library
include_directories(${PROJECT_SOURCE_DIR}/core/include/)
add_subdirectory(${PROJECT_SOURCE_DIR}/core)
include_recursive_dirs(${PROJECT_SOURCE_DIR}/core/*.h)

# link and build functions
function(link_libraries)
    # nothing to do
endfunction()

function(build)
    # nothing to do
endfunction()
