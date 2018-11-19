if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-gnu-zero-variadic-macro-arguments")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lc++ -lc++abi")
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
  if (GCC_VERSION VERSION_GREATER 7.0)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-strict-aliasing")
  endif()
endif()

check_unsupported_compiler_version()

add_definitions(-DTANGRAM_LINUX)

get_nextzen_api_key(NEXTZEN_API_KEY)
add_definitions(-DNEXTZEN_API_KEY="${NEXTZEN_API_KEY}")

find_package(OpenGL REQUIRED)

include(cmake/glfw.cmake)

# System font config
include(FindPkgConfig)
pkg_check_modules(FONTCONFIG REQUIRED "fontconfig")

add_executable(tangram
  platforms/linux/src/linuxPlatform.cpp
  platforms/linux/src/main.cpp
  platforms/common/platform_gl.cpp
  platforms/common/imgui_impl_glfw.cpp
  platforms/common/imgui_impl_opengl3.cpp
  platforms/common/urlClient.cpp
  platforms/common/linuxSystemFontHelper.cpp
  platforms/common/glfwApp.cpp
)

add_subdirectory(platforms/common/imgui)

target_include_directories(tangram
  PRIVATE
  platforms/common
  ${FONTCONFIG_INCLUDE_DIRS}
)

target_link_libraries(tangram
  PRIVATE
  tangram-core
  glfw
  imgui
  ${GLFW_LIBRARIES}
  ${OPENGL_LIBRARIES}
  ${FONTCONFIG_LDFLAGS}
  -lcurl
  -pthread
  # only used when not using external lib
  -ldl
)

target_compile_options(tangram
  PRIVATE
  -std=c++1y
  -fno-omit-frame-pointer
  -Wall
  -Wreturn-type
  -Wsign-compare
  -Wignored-qualifiers
  -Wtype-limits
  -Wmissing-field-initializers
)

add_resources(tangram "${PROJECT_SOURCE_DIR}/scenes")
