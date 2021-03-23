if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-gnu-zero-variadic-macro-arguments")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lc++ -lc++abi")
endif()

if (CMAKE_COMPILER_IS_GNUCC)
  message(STATUS "Using gcc ${CMAKE_CXX_COMPILER_VERSION}")
  if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 5.1)
    message(STATUS "USE CXX11_ABI")
    add_definitions("-D_GLIBCXX_USE_CXX11_ABI=1")
  endif()
  if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 7.0)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-strict-aliasing")
  endif()
endif()

check_unsupported_compiler_version()

find_package(Magnum CONFIG REQUIRED GL Sdl2Application)
find_package(CURL REQUIRED)

add_library(tangram SHARED
  platforms/common/platform_gl.cpp
  platforms/common/urlClient.cpp
  platforms/magnum/src/platform_magnum.cpp
)

target_include_directories(tangram
  PRIVATE
  platforms/common

)

target_link_libraries(tangram
  PRIVATE
  tangram-core
  ${CURL_LIBRARIES}
  Magnum::Magnum
  Magnum::GL
  Magnum::Sdl2Application # for flextGL
)


add_resources(tangram "${PROJECT_SOURCE_DIR}/scenes" "res")
