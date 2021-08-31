cmake_minimum_required(VERSION 3.14)
include(cmake/get_tangram_version.cmake)
include(cmake/vcpkg_setup.cmake)
get_tangram_version()
vcpkg_setup()

project(tangram VERSION ${TANGRAM_VERSION})

# set debug prefix for multi configuration generators
set(CMAKE_DEBUG_POSTFIX "d")

if(WIN32)
    set(DEFAULT_BIN_DIR bin)
    set(DEFAULT_LIB_DIR lib)
    set(DEFAULT_ARCHIVE_DIR lib)
else()
    include(GNUInstallDirs)
    set(DEFAULT_BIN_DIR ${CMAKE_INSTALL_BINDIR})
    set(DEFAULT_LIB_DIR ${CMAKE_INSTALL_LIBDIR})
    set(DEFAULT_ARCHIVE_DIR ${CMAKE_INSTALL_LIBDIR})
endif()
# https://cmake.org/cmake/help/latest/manual/cmake-buildsystem.7.html#runtime-output-artifacts
set(TANGRAM_BIN_DIR ${DEFAULT_BIN_DIR} CACHE STRING "Install directory for binaries")
# https://cmake.org/cmake/help/latest/manual/cmake-buildsystem.7.html#library-output-artifacts
set(TANGRAM_LIB_DIR ${DEFAULT_LIB_DIR} CACHE STRING "Install directory for libraries")
# https://cmake.org/cmake/help/latest/manual/cmake-buildsystem.7.html#archive-output-artifacts
set(TANGRAM_ARCHIVE_DIR ${DEFAULT_ARCHIVE_DIR} CACHE STRING "Install directory for archives")
set(INSTALL_CMAKE_DIR ${TANGRAM_LIB_DIR}/cmake/tangram CACHE STRING "Install directory of the cmake targets")

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

include(cmake/tangram_install.cmake)

if(CMAKE_BUILD_TYPE MATCHES Debug)
  add_definitions(-DDEBUG)
  add_definitions(-DLOG_LEVEL=3)
else()
  add_definitions(-DLOG_LEVEL=2)
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
  set_target_properties(platform_mock PROPERTIES CXX_STANDARD 14)
endif()

if(TANGRAM_BUILD_TESTS)
  message(STATUS "Building tests")
  add_subdirectory(tests)
endif()

if(TANGRAM_BUILD_BENCHMARKS)
  message(STATUS "Building benchmarks")
  add_subdirectory(bench)
endif()


# start package tangram
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/tangramConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY ExactVersion
)

set(INCLUDE_INSTALL_DIR include/)
configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/tangramConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/tangramConfig.cmake"
  INSTALL_DESTINATION ${INSTALL_CMAKE_DIR}
  PATH_VARS INCLUDE_INSTALL_DIR
)

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/tangramConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/tangramConfigVersion.cmake"
    DESTINATION ${INSTALL_CMAKE_DIR}
)

install(EXPORT tangramTargets
        FILE tangramTargets.cmake
        NAMESPACE tangram::
        DESTINATION ${INSTALL_CMAKE_DIR}
)

install(DIRECTORY vcpkg-ports/ DESTINATION ${CMAKE_INSTALL_PREFIX}/tangram-vcpkg-overlays)
