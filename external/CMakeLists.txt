cmake_minimum_required(VERSION 2.8.12)

if(USE_EXTERNAL_LIBS)
# http://www.cmake.org/Wiki/CMake/Tutorials/Exporting_and_Importing_Targets
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++1y")

if (CMAKE_COMPILER_IS_GNUCC)
  execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion
    OUTPUT_VARIABLE GCC_VERSION)

  if (GCC_VERSION VERSION_GREATER 5.1)
    add_definitions("-D_GLIBCXX_USE_CXX11_ABI=1")
  endif()
endif()

set(CMAKE_BUILD_TYPE Release)
set(EXPORTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/exports/${PLATFORM_TARGET})
endif()


## yaml-cpp ##
##############
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "Enable testing and parse tools")
set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "Enable contrib stuff in library")
add_subdirectory("yaml-cpp")


## css-color-parser-cpp ##
##########################
add_library(css-color-parser-cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/css-color-parser-cpp/csscolorparser.cpp)

target_include_directories(css-color-parser-cpp
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/css-color-parser-cpp)


## geojsonvt ##
###############
file(GLOB_RECURSE GEOJSON_VT_CPP_SOURCES "geojson-vt-cpp/src/*.cpp")
add_library(geojson-vt-cpp ${GEOJSON_VT_CPP_SOURCES})
target_include_directories(geojson-vt-cpp
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/geojson-vt-cpp/include
  # use our variant and geojsonvt_types.hpp
  # instead of the provided ones
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/src
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/include
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/include/mapbox)

set_target_properties(geojson-vt-cpp
  PROPERTIES COMPILE_FLAGS -DGEOJSONVT_CUSTOM_TAGS)

target_compile_options(geojson-vt-cpp
  PUBLIC
  -Wno-unknown-pragmas)


## duktape ##
#############
add_library(duktape
  ${CMAKE_CURRENT_SOURCE_DIR}/duktape/duktape.c)

target_compile_options(duktape PRIVATE
  -fstrict-aliasing
  -fomit-frame-pointer
  -std=c99
  -Wall)

target_include_directories(duktape
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/duktape)


## cmake exports ##
###################
if(USE_EXTERNAL_LIBS)
  export(TARGETS
    yaml-cpp
    css-color-parser-cpp
    geojson-vt-cpp
    duktape
    FILE ${EXPORTS_DIR}/core-dependencies.cmake)
endif()


## GLFW ##
##########
if(PLATFORM_LINUX OR PLATFORM_OSX)
  # configure GLFW to build only the library
  set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Build the GLFW example programs")
  set(GLFW_BUILD_TESTS OFF CACHE BOOL "Build the GLFW test programs")
  set(GLFW_BUILD_DOCS OFF CACHE BOOL "Build the GLFW documentation")
  set(GLFW_INSTALL OFF CACHE BOOL "Generate installation target")

  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/glfw)

  target_include_directories(glfw
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/glfw/include)

  target_link_libraries(glfw
    INTERFACE
    "${GLFW_LIBRARIES}"
    -ldl)

  target_compile_options(glfw
    PRIVATE
    -O3 -DNDEBUG)

  if(USE_EXTERNAL_LIBS)
    export(TARGETS glfw
      FILE ${EXPORTS_DIR}/glfw.cmake
      EXPORT_LINK_INTERFACE_LIBRARIES)
  endif()
endif()


## benchmark ##
###############
if((BENCHMARK OR USE_EXTERNAL_LIBS) AND (PLATFORM_LINUX OR PLATFORM_OSX))
  set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "")
  set(BENCHMARK_ENABLE_LTO OFF CACHE BOOL "")

  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/benchmark)
  target_compile_options(benchmark
    PRIVATE
    -O3 -DNDEBUG)

  if(USE_EXTERNAL_LIBS)
    export(TARGETS benchmark
      FILE ${EXPORTS_DIR}/benchmark.cmake)
  endif()
endif()

## alfons ##
############
#set(GLM_ROOT ${PROJECT_SOURCE_DIR}/core/include/glm)
set(GLM_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../core/include/glm)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/alfons)
