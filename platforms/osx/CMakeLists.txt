cmake_minimum_required(VERSION 3.2)

include(../../toolchains/utils.cmake)

check_unsupported_compiler_version()

add_definitions(-DTANGRAM_OSX)

# Build core library with dependencies.
add_subdirectory(../../core ./core)

if(TANGRAM_APPLICATION)

  if($ENV{CIRCLE_BUILD_NUM})
    add_definitions(-DBUILD_NUM_STRING="\($ENV{CIRCLE_BUILD_NUM}\)")
  endif()

  find_package(OpenGL REQUIRED)

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
    add_subdirectory(../common/glfw ./glfw)
    target_compile_options(glfw PRIVATE "-Wno-deprecated-declarations")
  endif()

  add_bundle_resources(RESOURCES ../../scenes "Resources")

  add_executable(tangram
    MACOSX_BUNDLE
    src/main.mm
    src/osxPlatform.mm
    ../common/platform_gl.cpp
    ../common/glfwApp.cpp
    ../common/appleAllowedFonts.mm
    ${RESOURCES}
  )

  target_include_directories(tangram
    PRIVATE
    ${GLFW_SOURCE_DIR}/include
    ../common
  )

  target_link_libraries(tangram
    PRIVATE
    tangram-core
    glfw
    ${GLFW_LIBRARIES}
    ${OPENGL_LIBRARIES}
  )

  target_compile_options(tangram
    PRIVATE
    -std=c++1y
    -stdlib=libc++
    -Wall
    -Wreturn-type
    -Wsign-compare
    -Wignored-qualifiers
    -Wtype-limits
    -Wmissing-field-initializers
  )

  # add resource files and property list
  set_target_properties(tangram PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist)

endif()
