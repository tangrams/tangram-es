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
  add_subdirectory(platforms/common/glfw)
  if(APPLE)
    # Turn off noisy warnings from clang on macOS.
    target_compile_options(glfw PRIVATE "-Wno-deprecated-declarations")
  endif()
endif()
