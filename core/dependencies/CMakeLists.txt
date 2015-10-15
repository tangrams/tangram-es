# css-color-parser-cpp #
########################

add_library(css-color-parser-cpp "css-color-parser-cpp/csscolorparser.cpp")
target_include_directories(css-color-parser-cpp
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/css-color-parser-cpp
)

## geojsonvt ##
###############

file(GLOB_RECURSE GEOJSON_VT_CPP_SOURCES "geojson-vt-cpp/src/*.cpp")
include_directories(${PROJECT_SOURCE_DIR}/${INCLUDE_DIR})
add_library(geojson-vt-cpp ${GEOJSON_VT_CPP_SOURCES})
target_include_directories(geojson-vt-cpp
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/geojson-vt-cpp/include
)

# yaml-cpp #
############

set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "Enable testing and parse tools")
set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "Enable contrib stuff in library")
add_subdirectory("yaml-cpp")


# duktape #
############

add_library(duktape "${CMAKE_CURRENT_SOURCE_DIR}/duktape/duktape.c")
#target_compile_options(duktape PRIVATE
#  -fstrict-aliasing
#  -fomit-frame-pointer
#  -std=c99
#  -Wall
#)
target_include_directories(duktape
  PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/duktape"
)
