# css-color-parser-cpp #
########################

add_library(css-color-parser-cpp "css-color-parser-cpp/csscolorparser.cpp")
target_include_directories(css-color-parser-cpp
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/css-color-parser-cpp
)

# benchmark #
#############

if(BENCHMARK)
  option(BENCHMARK_ENABLE_TESTING OFF)
  option(BENCHMARK_ENABLE_LTO OFF)
  set(CMAKE_BUILD_TYPE Release)
  add_subdirectory(benchmark)
endif()

# libtess2 #
############

add_definitions(-DNDEBUG)
file(GLOB_RECURSE LIBTESS2_SOURCES "libtess2/Source/*.c")
add_library(tess2 ${LIBTESS2_SOURCES})
target_include_directories(tess2
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/libtess2/Include
)

# yaml-cpp #
############

set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "Enable testing and parse tools")
add_subdirectory("yaml-cpp")
