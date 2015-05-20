add_definitions(-DPLATFORM_ANDROID)

if(NOT MAKE_BUILD_TOOL)
    message(SEND_ERROR "Provide ndk make, located in <NDK_ROOT>/prebuilt/<OS>/bin/, -DMAKE_BUILD_TOOL=<NDK_PREBUILT_BIN>/make")
    return()
else()
    set(CMAKE_BUILD_TOOL ${MAKE_BUILD_TOOL})
    message(STATUS "Will use make prebuilt tool located at : ${CMAKE_BUILD_TOOL}")
endif()

# configurations
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11 -pedantic -llog -lz -landroid")
set(CXX_FLAGS_DEBUG "${CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fPIC")

# adding manually the two jni c++ files
set(ADDITIONNAL_TARGET_DEPENDENT_SRC_FILES 
     ${CMAKE_SOURCE_DIR}/android/jni/jniExports.cpp
     ${CMAKE_SOURCE_DIR}/android/jni/platform_android.cpp)

# load core library
set(INSTALL_CORE_LIBRARY "ON")
set(CORE_LIB_TYPE SHARED)
set(CORE_INSTALLATION_PATH ${CMAKE_SOURCE_DIR}/android/libs/${ANDROID_ABI})
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
