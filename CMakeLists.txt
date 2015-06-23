cmake_minimum_required(VERSION 2.8)
project(tangram)

include(${PROJECT_SOURCE_DIR}/toolchains/utils.cmake)

# cmake output configuration
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# platform lookup
set(SUPPORTED_TARGETS darwin ios android raspberrypi linux)

if(NOT PLATFORM_TARGET)
    string(TOLOWER "${CMAKE_SYSTEM_NAME}" varplatform)
    string(TOUPPER "${CMAKE_SYSTEM_NAME}" VARPLATFORM)

    message(STATUS "No target defined (give as parameter -DPLATFORM_TARGET=platform_name).")
    message(STATUS "Targets available: ${SUPPORTED_TARGETS}.")
    message(STATUS "Will use your OS: ${varplatform}.")

    set(PLATFORM_TARGET ${varplatform})
else()
    string(TOLOWER "${PLATFORM_TARGET}" varplatform)
    string(TOUPPER "${PLATFORM_TARGET}" VARPLATFORM)

    message(STATUS "Platform chosen : ${VARPLATFORM}")
endif()

list(FIND SUPPORTED_TARGETS ${varplatform} target_in_list)

if(target_in_list EQUAL -1)
    message(SEND_ERROR "${varplatform} not in supported targets: ${SUPPORTED_TARGETS}")
    return()
endif()

if(UNIT_TESTS) 
    include(${PROJECT_SOURCE_DIR}/toolchains/unitTests.cmake)
#   include(${PROJECT_SOURCE_DIR}/toolchains/linux.tests.cmake)
    return()
endif()

# setting up cache variable platform 
set(PLATFORM ${varplatform} CACHE INTERNAL "platform" FORCE)

# cmake inclusions and definitions
# set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/build/modules)
set(CMAKE_TARGET_FILE ${PROJECT_SOURCE_DIR}/toolchains/${PLATFORM_TARGET}.cmake)

message(STATUS "Including ${varplatform} dependent cmake file : ${CMAKE_TARGET_FILE}")
include(${CMAKE_TARGET_FILE})

# dependent platform build
build()

link_libraries()
