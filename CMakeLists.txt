cmake_minimum_required(VERSION 3.0)

project(tangram)

if(NOT EXISTS "${PROJECT_SOURCE_DIR}/external/alfons/deps/harfbuzz-icu-freetype/CMakeLists.txt")
  message(SEND_ERROR "Missing submodules - Please run:\n 'git submodule update --init --recursive'")
  return()
endif()

set(APPLICATION ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

include(${PROJECT_SOURCE_DIR}/toolchains/utils.cmake)

# cmake output configuration
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# platform lookup
set(SUPPORTED_TARGETS darwin ios android raspberrypi linux tizen)

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

# setting up cache variable platform
set(PLATFORM ${varplatform} CACHE INTERNAL "platform" FORCE)

# cmake inclusions and definitions
set(CMAKE_TARGET_FILE ${PROJECT_SOURCE_DIR}/toolchains/${PLATFORM_TARGET}.cmake)

message(STATUS "Including ${varplatform} dependent cmake file : ${CMAKE_TARGET_FILE}")

set(EXTERNAL_LIBS_DIR ${CMAKE_SOURCE_DIR}/external/exports/${PLATFORM_TARGET}
    CACHE INTERNAL "external-libs" FORCE)

## set platform configuration, build library and application.
# adds targets:
# - core
# - tangram
# - copy_resources
include(${CMAKE_TARGET_FILE})

if(BENCHMARK OR UNIT_TESTS)
  add_library(platform_mock
    ${PROJECT_SOURCE_DIR}/tests/src/platform_mock.cpp
    ${PROJECT_SOURCE_DIR}/tests/src/gl_mock.cpp)

  target_include_directories(platform_mock
    PUBLIC ${CORE_LIBRARIES_INCLUDE_DIRS})

  target_compile_definitions(platform_mock
    PUBLIC -DUNIT_TESTS)

  target_link_libraries(platform_mock
    PUBLIC core)

endif()

if(UNIT_TESTS)
  message(STATUS "Build with tests")
  add_subdirectory(${PROJECT_SOURCE_DIR}/tests)
endif()

if(BENCHMARK)
  message(STATUS "Build with benchmarks")
  add_subdirectory(${PROJECT_SOURCE_DIR}/bench)
endif()
