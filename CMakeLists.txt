cmake_minimum_required(VERSION 3.2)

project(tangram)

# Options
option(TANGRAM_USE_JSCORE "Use system libraries for JavaScriptCore and enable it on iOS and macOS" OFF)
option(TANGRAM_USE_JSCORE_STATIC "Build with pre-compiled static libraries for JavaScriptCore" OFF)

option(TANGRAM_USE_SYSTEM_FONT_LIBS "Use system libraries Freetype, ICU and Harfbuzz via pkgconfig" OFF)
option(TANGRAM_USE_SYSTEM_GLFW_LIBS "Use system libraries for GLFW3 via pkgconfig" OFF)
option(TANGRAM_USE_SYSTEM_SQLITE_LIBS "Use system libraries for SQLite via pkgconfig" OFF)
option(TANGRAM_MBTILES_DATASOURCE "Build MBTiles Datasource" ON)

option(TANGRAM_BUILD_TESTS "Build unit tests" OFF)
option(TANGRAM_BUNDLE_TESTS "Compile all tests into a single binary" ON)
option(TANGRAM_BUILD_BENCHMARKS "Build benchmarks" OFF)
option(TANGRAM_DEV_MODE "For developers only: Don't omit the frame pointer" OFF)

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
if(TANGRAM_DEV_MODE AND TANGRAM_BUILD_BENCHMARKS)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-omit-frame-pointer")
endif()

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

# Choose JavaScript implementation.
if (TANGRAM_IOS OR TANGRAM_OSX OR TANGRAM_USE_JSCORE_STATIC)
  set(TANGRAM_JSCORE_AVAILABLE ON)
endif()
if (TANGRAM_JSCORE_AVAILABLE AND TANGRAM_USE_JSCORE)
  set(TANGRAM_JSCORE_ENABLED ON)
endif()

# Add core library.
add_subdirectory(core)

if(TANGRAM_BUILD_BENCHMARKS OR TANGRAM_BUILD_TESTS)
  add_library(platform_mock
    tests/src/mockPlatform.cpp
    tests/src/gl_mock.cpp
  )
  target_include_directories(platform_mock PUBLIC tests/src)
  target_compile_definitions(platform_mock PUBLIC -DUNIT_TESTS)
  target_link_libraries(platform_mock PUBLIC tangram-core)
endif()

if(TANGRAM_BUILD_TESTS)
  message(STATUS "Building tests")
  add_subdirectory(tests)
endif()

if(TANGRAM_BUILD_BENCHMARKS)
  message(STATUS "Building benchmarks")
  add_subdirectory(bench)
endif()
