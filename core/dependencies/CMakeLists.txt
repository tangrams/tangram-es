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
