if(NOT TIZEN_SDK)
    message(SEND_ERROR "Set tizen sdk path: $TIZEN_SDK or -DTIZEN_SDK")
    return()
endif()

# include(${CMAKE_SOURCE_DIR}/toolchains/tizen.toolchain.cmake)

# set for test in other cmake files
set(PLATFORM_TIZEN ON)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fPIC")

# global compile options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++1y -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer")

if (NOT ${TIZEN_DEVICE})
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=i486")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=i486")
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-gnu-zero-variadic-macro-arguments")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif()

if (CMAKE_COMPILER_IS_GNUCC)
  execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion
    OUTPUT_VARIABLE GCC_VERSION)
  string(REGEX MATCHALL "[0-9]+" GCC_VERSION_COMPONENTS ${GCC_VERSION})
  list(GET GCC_VERSION_COMPONENTS 0 GCC_MAJOR)
  list(GET GCC_VERSION_COMPONENTS 1 GCC_MINOR)

  message(STATUS "Using gcc ${GCC_VERSION}")
  if (GCC_VERSION VERSION_GREATER 5.1)
    message(STATUS "USE CXX11_ABI")
    add_definitions("-D_GLIBCXX_USE_CXX11_ABI=1")
  endif()
endif()

check_unsupported_compiler_version()

# compile definitions (adds -DPLATFORM_LINUX)
set(CORE_COMPILE_DEFS PLATFORM_TIZEN)

# load core library
add_subdirectory(${PROJECT_SOURCE_DIR}/core)

set(LIB_NAME tangram) # in order to have libtangram.so

# add sources and include headers
find_sources_and_include_directories(
  ${PROJECT_SOURCE_DIR}/tizen/inc/*.h
  ${PROJECT_SOURCE_DIR}/tizen/src/*.cpp)

add_library(${LIB_NAME} SHARED ${SOURCES})

# link to the core library, forcing all symbols to be added
# (whole-archive must be turned off after core so that lc++ symbols aren't duplicated)
target_link_libraries(${LIB_NAME}
  PUBLIC
  "-Wl,-whole-archive"
  ${CORE_LIBRARY}
  "-Wl,-no-whole-archive")

target_compile_options(${LIB_NAME}
  PUBLIC
  -fPIC)
