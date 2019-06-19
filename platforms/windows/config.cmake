check_unsupported_compiler_version()

add_definitions(-DTANGRAM_WINDOWS)

find_package(OpenGL REQUIRED)

if(NOT DEFINED CURL_LIBRARIES OR NOT DEFINED CURL_INCLUDE_DIRS)
  find_package(CURL REQUIRED)
endif()

include(cmake/glfw.cmake)

add_executable(tangram
  platforms/windows/src/windowsPlatform.cpp
  platforms/windows/src/main.cpp
  platforms/common/platform_gl.cpp
  platforms/common/urlClient.cpp
  platforms/common/glfwApp.cpp
  platforms/common/imgui_impl_glfw.cpp
  platforms/common/imgui_impl_opengl3.cpp
  platforms/common/glfw/deps/glad.c
)

add_resources(tangram "${PROJECT_SOURCE_DIR}/scenes" "res")

add_subdirectory(platforms/common/imgui)

target_include_directories(tangram
  PRIVATE
  platforms/common
  platforms/common/glfw/deps
  ${CURL_INCLUDE_DIRS}
)

target_link_libraries(tangram
  PRIVATE
  tangram-core
  glfw
  imgui
  ${GLFW_LIBRARIES}
  ${OPENGL_LIBRARIES}
  ${CURL_LIBRARIES}
  -pthread
  wsock32 ws2_32 crypt32 wldap32
)

target_compile_options(tangram
  PRIVATE
  -std=c++14
  -fno-omit-frame-pointer
  -Wall
  -Wreturn-type
  -Wsign-compare
  -Wignored-qualifiers
  -Wtype-limits
  -Wmissing-field-initializers
)

get_nextzen_api_key(NEXTZEN_API_KEY)
target_compile_definitions(tangram PRIVATE NEXTZEN_API_KEY="${NEXTZEN_API_KEY}")
