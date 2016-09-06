add_definitions(-DPLATFORM_ANDROID)

if(NOT MAKE_BUILD_TOOL)
    message(SEND_ERROR "Provide ndk make, located in <NDK_ROOT>/prebuilt/<OS>/bin/, -DMAKE_BUILD_TOOL=<NDK_PREBUILT_BIN>/make")
    return()
else()
    set(CMAKE_BUILD_TOOL ${MAKE_BUILD_TOOL})
    message(STATUS "Will use make prebuilt tool located at : ${CMAKE_BUILD_TOOL}")
endif()

FILE(READ "$ENV{ANDROID_NDK}/RELEASE.TXT" NDK_VERSION_STRING)
string(FIND ${NDK_VERSION_STRING} "r10e" NDK_MATCH)
if(${NDK_MATCH} LESS 0)
  message(SEND_ERROR "Please use NDK version r10e as $ANDROID_NDK")
  return()
endif()

# check for unsupported compilers
check_unsupported_compiler_version()

# configurations
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y -pedantic")

# build external dependencies
add_subdirectory(${PROJECT_SOURCE_DIR}/external)

# load core library
add_subdirectory(${PROJECT_SOURCE_DIR}/core)

set(ANDROID_PROJECT_DIR ${PROJECT_SOURCE_DIR}/android/tangram)

if(CMAKE_BUILD_TYPE MATCHES Debug)
include(toolchains/android.gdb.cmake)
android_ndk_gdb_enable()
endif()

set(LIB_NAME tangram) # in order to have libtangram.so

add_library(${LIB_NAME} SHARED
  ${CMAKE_SOURCE_DIR}/core/common/platform_gl.cpp
  ${CMAKE_SOURCE_DIR}/android/tangram/jni/jniExports.cpp
  ${CMAKE_SOURCE_DIR}/android/tangram/jni/platform_android.cpp)


# https://code.google.com/p/android/issues/detail?id=68779
# link atomic support, should be fixed after r10e
if (ANDROID_ABI MATCHES armeabi OR
    ANDROID_ABI MATCHES mips)
  set(ATOMIC_LIB atomic)
endif()

target_link_libraries(${LIB_NAME}
  PUBLIC
  ${CORE_LIBRARY}
  # android libaries
  ${ATOMIC_LIB}
  GLESv2 log z android)

target_compile_options(${LIB_NAME}
  PUBLIC
  -fPIC)

if(CMAKE_BUILD_TYPE MATCHES Debug)
android_ndk_gdb_debuggable(${LIB_NAME})
endif()

# install to android library dir
set(LIB_INSTALLATION_PATH ${CMAKE_SOURCE_DIR}/android/tangram/libs/${ANDROID_ABI})


install(TARGETS ${LIB_NAME} DESTINATION ${LIB_INSTALLATION_PATH})
