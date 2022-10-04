# Build GLFW.
if(TANGRAM_USE_SYSTEM_GLFW_LIBS)
  include(FindPkgConfig)
  pkg_check_modules(GLFW REQUIRED glfw3)
else()
  # configure GLFW to build only the library
  set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Build the GLFW example programs")
  set(GLFW_BUILD_TESTS OFF CACHE BOOL "Build the GLFW test programs")
  set(GLFW_BUILD_DOCS OFF CACHE BOOL "Build the GLFW documentation")
  set(GLFW_INSTALL OFF CACHE BOOL "Generate installation target")
  # Select Windowing subsystem
  option(TANGRAM_USE_WAYLAND "Use Wayland for Linux windowing system" OFF)
  option(TANGRAM_USE_X11 "Use x11 for Linux windowing system" ON)
  set(GLFW_BUILD_WAYLAND ${TANGRAM_USE_WAYLAND})
  set(GLFW_BUILD_X11 ${TANGRAM_USE_X11})
  add_subdirectory(platforms/common/glfw)
  if(APPLE)
    # Turn off noisy warnings from clang on macOS.
    target_compile_options(glfw PRIVATE "-Wno-deprecated-declarations")
  endif()
endif()
