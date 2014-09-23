cmake_minimum_required(VERSION 2.8)
project(tangram)

# platform lookup
set(SUPPORTED_TARGETS darwin ios)

if(NOT PLATFORM_TARGET)
    string(TOLOWER "${CMAKE_SYSTEM_NAME}" varplatform)
    string(TOUPPER "${CMAKE_SYSTEM_NAME}" VARPLATFORM)

    message("No target defined (give as parameter -DPLATFORM_TARGET=platform_name).")
    message("Targets available: ${SUPPORTED_TARGETS}.")
    message("Will use your OS: ${varplatform}.")

    set(PLATFORM_TARGET ${varplatform})
else()
    string(TOLOWER "${PLATFORM_TARGET}" varplatform)
    string(TOUPPER "${PLATFORM_TARGET}" VARPLATFORM)

    message("Platform chosen : ${varplatform}")
endif()

list(FIND SUPPORTED_TARGETS ${varplatform} target_in_list)

if(target_in_list EQUAL -1)
    message(SEND_ERROR "${varplatform} not in supported targets: ${SUPPORTED_TARGETS}")
    return()
endif()

# cmake inclusions and definitions
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/build/modules)
set(CMAKE_TARGET_FILE ${PROJECT_SOURCE_DIR}/build/toolchains/${PLATFORM_TARGET}.cmake)

include(${PROJECT_SOURCE_DIR}/build/utils/utils.cmake)

message("Including ${varplatform} dependent cmake file : ${CMAKE_TARGET_FILE}")
include(${CMAKE_TARGET_FILE})

# dependent platform build
build()
link_libraries()
