set(SOURCE_FILE csscolorparser/csscolorparser.cpp)
#set(HEADER_FILE csscolorparser.hpp)
include_directories(csscolorparser)

add_library(libcsscolorparser ${SOURCE_FILE})

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

include_directories("libtess2/Include")
add_library(libtess2 ${LIBTESS2_SOURCES})

# yaml-cpp #
############

set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "Enable testing and parse tools")
add_subdirectory("yaml-cpp")
