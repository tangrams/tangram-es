cmake_minimum_required(VERSION 3.2)

project(tangram)

# Options
option(TANGRAM_USE_SYSTEM_FONT_LIBS "Use system libraries Freetype, ICU and Harfbuzz via pkgconfig" OFF)
option(TANGRAM_USE_SYSTEM_GLFW_LIBS "Use system libraries for GLFW3 via pkgconfig" OFF)

message(STATUS "Build type configuration: ${CMAKE_BUILD_TYPE}")

# Check that submodules are present.
if(NOT EXISTS "${PROJECT_SOURCE_DIR}/core/deps/harfbuzz-icu-freetype/harfbuzz/README")
  message(SEND_ERROR "Missing submodules - Please run:\n 'git submodule update --init'")
  return()
endif()

if(CMAKE_BUILD_TYPE MATCHES Debug)
  add_definitions(-DDEBUG)
  add_definitions(-DLOG_LEVEL=3)
else()
  add_definitions(-DLOG_LEVEL=2)
endif()

if($ENV{CIRCLE_BUILD_NUM})
  add_definitions(-DBUILD_NUM_STRING="\($ENV{CIRCLE_BUILD_NUM}\)")
endif()

include(cmake/utils.cmake)

# If target platform isn't specified, use the local platform.
if(NOT TANGRAM_PLATFORM)
  message(STATUS "No platform target specified. Choose a target with -DTANGRAM_PLATFORM=platform.")
  string(TOLOWER "${CMAKE_SYSTEM_NAME}" SYSTEM_NAME)
  if (${SYSTEM_NAME} STREQUAL "darwin")
    set(TANGRAM_PLATFORM "osx")
  else()
    set(TANGRAM_PLATFORM ${SYSTEM_NAME})
  endif()
  message(STATUS "Using your OS as the platform target: ${TANGRAM_PLATFORM}.")
endif()

# Configure platform target.
if(TANGRAM_PLATFORM)
  set(TANGRAM_PLATFORM_CONFIG_FILE "platforms/${TANGRAM_PLATFORM}/config.cmake")
  if(EXISTS "${PROJECT_SOURCE_DIR}/${TANGRAM_PLATFORM_CONFIG_FILE}")
    message(STATUS "Including platform CMake config file: ${TANGRAM_PLATFORM_CONFIG_FILE}")
  else()
    message(SEND_ERROR "Cannot find CMake config file for platform: ${TANGRAM_PLATFORM}. Looked for: ${TANGRAM_PLATFORM_CONFIG_FILE}")
    return()
  endif()
  # Cache platform target.
  set(TANGRAM_PLATFORM ${TANGRAM_PLATFORM} CACHE INTERNAL "Tangram platform target" FORCE)
  # Include the platform config file.
  include(${TANGRAM_PLATFORM_CONFIG_FILE})
endif()

# Add core library.
add_subdirectory(core)

if(BENCHMARK OR UNIT_TESTS)
  add_library(platform_mock
    tests/src/mockPlatform.cpp
    tests/src/gl_mock.cpp
  )
  target_include_directories(platform_mock PUBLIC tests/src)
  target_compile_definitions(platform_mock PUBLIC -DUNIT_TESTS)
  target_link_libraries(platform_mock PUBLIC tangram-core)
endif()

if(UNIT_TESTS)
  message(STATUS "Building tests")
  add_subdirectory(tests)
endif()

if(BENCHMARK)
  message(STATUS "Building benchmarks")
  add_subdirectory(bench)
endif()
