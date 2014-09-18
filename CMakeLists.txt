cmake_minimum_required(VERSION 2.8)
project(tangram)

set(SUPPORTED_TARGETS darwin)

if(NOT PLATFORM_TARGET)
    string(TOLOWER "${CMAKE_SYSTEM_NAME}" varplatform)
    string(TOUPPER "${CMAKE_SYSTEM_NAME}" VARPLATFORM)

    message("No target defined (give as parameter -DPLATFOR_TARGET=platform_name).")
    message("Targets available: ${SUPPORTED_TARGETS}.")
    message("Will use your OS: ${varplatform}.")

    set(PLATFORM_TARGET ${varplatform})
else()
    string(TOLOWER "${TARGET}" varplatform)
    string(TOUPPER "${TARGET}" VARPLATFORM)
endif()

list(FIND SUPPORTED_TARGETS ${varplatform} target_in_list)

if(target_in_list EQUAL -1)
    message(SEND_ERROR "${varplatform} not in supported targets: ${SUPPORTED_TARGETS}")
    return()
endif()

# compilation options
set(CMAKE_CXX_FLAGS "-Wall -stdlib=libc++ -std=c++0x -g -O0")
set(EXECUTABLE_NAME "tangram")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/build/modules")

include(${PROJECT_SOURCE_DIR}/build/toolchains/${PLATFORM_TARGET}.cmake)

add_executable(${EXECUTABLE_NAME} ${SOURCES})

link_libraries()
