cmake_minimum_required(VERSION 2.8.12)

## yaml-cpp ##
##############
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "")
set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "")
set(YAML_CPP_INSTALL OFF CACHE BOOL "")
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/yaml-cpp)

target_include_directories(yaml-cpp
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/yaml-cpp/include
)

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
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/mapbox
  ${CMAKE_CURRENT_SOURCE_DIR}/../include/tangram)

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


if(NOT USE_SYSTEM_FONT_LIBS)
  ## Harfbuzz - ICU-Common - UCDN - Freetype2 ##
  ##############################################
  set(HARFBUZZ_BUILD_ICU ON CACHE BOOL "Enable building of ICU")
  set(HARFBUZZ_BUILD_UCDN ON CACHE BOOL "Enable building of UCDN")
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/harfbuzz-icu-freetype)

  message(STATUS "harfbuzz" ${HARFBUZZ_LIBRARIES})

  set(ALFONS_DEPS_LIBRARIES
    ${ALFONS_DEPS_LIBRARIES}
    harfbuzz ${HARFBUZZ_LIBRARIES}
    CACHE INTERNAL "alfons-libs" FORCE)
endif()

## alfons ##
############
set(GLM_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/glm)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/alfons)

## SQLiteCpp ##
###############
set(SQLITECPP_RUN_CPPLINT OFF CACHE BOOL "")
set(SQLITECPP_RUN_CPPCHECK OFF CACHE BOOL "")

add_subdirectory(SQLiteCpp)
