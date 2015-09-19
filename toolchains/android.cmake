add_definitions(-DPLATFORM_ANDROID)

if(NOT MAKE_BUILD_TOOL)
    message(SEND_ERROR "Provide ndk make, located in <NDK_ROOT>/prebuilt/<OS>/bin/, -DMAKE_BUILD_TOOL=<NDK_PREBUILT_BIN>/make")
    return()
else()
    set(CMAKE_BUILD_TOOL ${MAKE_BUILD_TOOL})
    message(STATUS "Will use make prebuilt tool located at : ${CMAKE_BUILD_TOOL}")
endif()

# configurations
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++1y -pedantic -llog -lz -landroid")
set(CXX_FLAGS_DEBUG "${CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fPIC")

# For CMake 3.0
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # using regular Clang or AppleClang
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-gnu-anonymous-struct -Wno-nested-anon-types -Wno-gnu-zero-variadic-macro-arguments")
endif()

# adding manually the two jni c++ files
set(ADDITIONAL_TARGET_DEPENDENT_SRC_FILES
     ${CMAKE_SOURCE_DIR}/android/tangram/jni/jniExports.cpp
     ${CMAKE_SOURCE_DIR}/android/tangram/jni/jniGenerated.cpp
     ${CMAKE_SOURCE_DIR}/android/tangram/jni/platform_android.cpp)

# load core library
set(INSTALL_CORE_LIBRARY "ON")
set(CORE_LIB_TYPE SHARED)
set(CORE_INSTALLATION_PATH ${CMAKE_SOURCE_DIR}/android/tangram/libs/${ANDROID_ABI})
set(CORE_LIB_DEPS GLESv2)
set(CORE_LIB_NAME tangram) # in order to have libtangram.so

add_subdirectory(${PROJECT_SOURCE_DIR}/core)
include_directories(${CORE_INCLUDE_DIRS})

# link and build functions
function(link_libraries)
    # nothing to do
endfunction()

function(build)
    # nothing to do
endfunction()
