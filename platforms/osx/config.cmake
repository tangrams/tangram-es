check_unsupported_compiler_version()

add_definitions(-DTANGRAM_OSX)

find_package(OpenGL REQUIRED)

include(cmake/glfw.cmake)

# Tell SQLiteCpp to not build its own copy of SQLite, we will use the system library instead.
set(SQLITECPP_INTERNAL_SQLITE OFF CACHE BOOL "")

add_bundle_resources(RESOURCES "${PROJECT_SOURCE_DIR}/scenes" "Resources")

add_executable(tangram
  MACOSX_BUNDLE
  platforms/osx/src/main.mm
  platforms/osx/src/osxPlatform.mm
  platforms/common/platform_gl.cpp
  platforms/common/imgui_impl_glfw.cpp
  platforms/common/imgui_impl_opengl3.cpp
  platforms/common/glfwApp.cpp
  platforms/common/appleAllowedFonts.mm
  ${RESOURCES}
)

add_subdirectory(platforms/common/imgui)

target_include_directories(tangram
  PRIVATE
  platforms/common
)

target_link_libraries(tangram
  PRIVATE
  tangram-core
  imgui
  glfw
  ${GLFW_LIBRARIES}
  ${OPENGL_LIBRARIES}
)

target_compile_options(tangram
  PRIVATE
  -std=c++1y
  -stdlib=libc++
  -fobjc-arc
  -Wall
  -Wreturn-type
  -Wsign-compare
  -Wignored-qualifiers
  -Wtype-limits
  -Wmissing-field-initializers
)

# add resource files and property list
set_target_properties(tangram PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${PROJECT_SOURCE_DIR}/platforms/osx/Info.plist")
