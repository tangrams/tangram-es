add_definitions(-DPLATFORM_ANDROID)

if(NOT MAKE_BUILD_TOOL)
    message(SEND_ERROR "Provide ndk make, located in <NDK_ROOT>/prebuilt/<OS>/bin/, -DMAKE_BUILD_TOOL=<NDK_PREBUILT_BIN>/make")
    return()
else()
    set(CMAKE_BUILD_TOOL ${MAKE_BUILD_TOOL})
    message(STATUS "Will use make prebuilt tool located at : ${CMAKE_BUILD_TOOL}")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y -pedantic")

# configurations
#set(CXX_FLAGS "${CXX_FLAGS} -Wall -std=c++1y -pedantic")
#set(CXX_FLAGS_DEBUG "${CXX_FLAGS_DEBUG} -g -O0")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fPIC")

# build external dependencies
add_subdirectory(${PROJECT_SOURCE_DIR}/external)

# load core library
add_subdirectory(${PROJECT_SOURCE_DIR}/core)

set(LIB_NAME tangram) # in order to have libtangram.so

add_library(${LIB_NAME} SHARED
  ${CMAKE_SOURCE_DIR}/android/tangram/jni/jniExports.cpp
  ${CMAKE_SOURCE_DIR}/android/tangram/jni/jniGenerated.cpp
  ${CMAKE_SOURCE_DIR}/android/tangram/jni/platform_android.cpp)

target_link_libraries(${LIB_NAME}
  PUBLIC
  ${CORE_LIBRARY}
  # android libaries
  GLESv2 log z android)

target_compile_options(${LIB_NAME}
  PUBLIC
  -fPIC)


# install to android library dir
set(LIB_INSTALLATION_PATH ${CMAKE_SOURCE_DIR}/android/tangram/libs/${ANDROID_ABI})


# from http://www.stellarium.org/wiki/index.php/Building_for_Android
if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
  message(STATUS " >>> DEBUG <<< ")

  # 1. generate a fake Android.mk
  file(WRITE ${CMAKE_SOURCE_DIR}/android/tangram/jni/Android.mk "APP_ABI := ${ANDROID_ABI}\n")

  # 2. generate gdb.setup
  get_directory_property(INCLUDE_DIRECTORIES DIRECTORY . INCLUDE_DIRECTORIES)
  message(STATUS "Directories >> ${INCLUDE_DIRECTORIES} <<")

  string(REGEX REPLACE ";" " " INCLUDE_DIRECTORIES "${INCLUDE_DIRECTORIES}")
  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/libs/${ARM_TARGET}/gdb.setup
    "set solib-search-path ${CMAKE_CURRENT_BINARY_DIR}/obj/local/${ARM_TARGET}\n")

  file(APPEND ${LIB_INSTALLATION_PATH}/gdb.setup
    "directory ${INCLUDE_DIRECTORIES}\n")

  # 3. copy gdbserver executable
  file(COPY ${ANDROID_NDK}/prebuilt/android-arm/gdbserver/gdbserver
    DESTINATION ${LIB_INSTALLATION_PATH}/)
else()
  message(STATUS " >>> NO DEBUG <<< ")

endif()

install(TARGETS ${LIB_NAME} DESTINATION ${LIB_INSTALLATION_PATH})
