add_definitions(-DPLATFORM_ANDROID)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Wall -stdlib=libc++ -std=c++0x")
set(CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -pedantic -fPIC" )
set(EXECUTABLE_NAME "tangram.out")
set(ANDROID_TARGET "android-19")
set(ANDROID_ARCH "arch-x86")


if(NOT MAKE_BUILD_TOOL)
	message(SEND_ERROR "Provide ndk make, located in <NDK_ROOT>/prebuilt/<OS>/bin/, -DMAKE_BUILD_TOOL=<NDK_PREBUILT_BIN>/make")
	return()
else()
	set(CMAKE_BUILD_TOOL ${MAKE_BUILD_TOOL})
	message(STATUS "Will use make prebuilt tool located at : ${CMAKE_BUILD_TOOL}")
endif()

set(CORE_LIB_TYPE SHARED)
set(CORE_LIB_DEPS GLESv2)

#message(STATUS "Including : $ENV{NDK_ROOT}/platforms/${ANDROID_TARGET}/${ANDROID_ARCH}/usr/include/")

# MUST have NDK_ROOT in environment variables (todo : test if it already exists)
#include_directories($ENV{NDK_ROOT}/platforms/${ANDROID_TARGET}/${ANDROID_ARCH}/usr/include/)

include_directories(${PROJECT_SOURCE_DIR}/core/include/)
add_subdirectory(${PROJECT_SOURCE_DIR}/core)
include_recursive_dirs(${PROJECT_SOURCE_DIR}/core/*.h)

# link and build functions
function(link_libraries)
	message("link android")
endfunction()

function(build)
	message("build android")
endfunction()
