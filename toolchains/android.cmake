add_definitions(-DPLATFORM_ANDROID)

# build external dependencies
add_subdirectory(${PROJECT_SOURCE_DIR}/external)

# load core library
add_subdirectory(${PROJECT_SOURCE_DIR}/core)

set(ANDROID_PROJECT_DIR ${PROJECT_SOURCE_DIR}/android/tangram)

if(CMAKE_BUILD_TYPE MATCHES Debug)
 if(POLICY CMP0026)
   cmake_policy(SET CMP0026 OLD)
 endif()
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
