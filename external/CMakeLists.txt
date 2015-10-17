### http://www.cmake.org/Wiki/CMake/Tutorials/Exporting_and_Importing_Targets

cmake_minimum_required(VERSION 2.8.12)

if(USE_EXTERNAL_LIBS)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++1y")
set(CMAKE_BUILD_TYPE Release)
endif()

set(EXPORTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/exports/${PLATFORM_TARGET})



## yaml-cpp ##
##############
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "Enable testing and parse tools")
set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "Enable contrib stuff in library")
add_subdirectory("yaml-cpp")

if(USE_EXTERNAL_LIBS)
  export(TARGETS yaml-cpp
    FILE ${EXPORTS_DIR}/yaml-cpp.cmake)
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
if(PLATFORM_LINUX OR PLATFORM_OSX)
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
