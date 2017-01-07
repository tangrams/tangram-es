# set for test in other cmake files
set(PLATFORM_WINDOWS ON)

# global compile options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -utf-8 -wd4514 -wd4710 -wd4996 -std:c++14")

check_unsupported_compiler_version()

# compile definitions (adds -DPLATFORM_WINDOWS)
set(CORE_COMPILE_DEFS PLATFORM_WINDOWS _USE_MATH_DEFINES TANGRAM_USE_GLPROXY GLPROXY_STATIC_LIB)

# load core library
add_subdirectory(${PROJECT_SOURCE_DIR}/core)


if(APPLICATION)

  set(EXECUTABLE_NAME "tangram")

  # configure GLFW to build only the library
  set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Build the GLFW example programs")
  set(GLFW_BUILD_TESTS OFF CACHE BOOL "Build the GLFW test programs")
  set(GLFW_BUILD_DOCS OFF CACHE BOOL "Build the GLFW documentation")
  set(GLFW_INSTALL OFF CACHE BOOL "Generate installation target")
  add_subdirectory(${PROJECT_SOURCE_DIR}/platforms/common/glfw)

  # add sources and include headers
  find_sources_and_include_directories(
    ${PROJECT_SOURCE_DIR}/platforms/windows/src/*.h
    ${PROJECT_SOURCE_DIR}/platforms/windows/src/*.cpp)

  add_executable(${EXECUTABLE_NAME}
    ${SOURCES}
    ${PROJECT_SOURCE_DIR}/core/deps/glproxy/src/dispatch_common.c
    ${PROJECT_SOURCE_DIR}/core/deps/glproxy/src/dispatch_generated.c
    ${PROJECT_SOURCE_DIR}/platforms/common/platform_gl.cpp
    ${PROJECT_SOURCE_DIR}/platforms/common/urlClient.cpp
    ${PROJECT_SOURCE_DIR}/platforms/common/urlClientWindows.cpp
    ${PROJECT_SOURCE_DIR}/platforms/common/glfwApp.cpp
  )

  target_compile_definitions(${EXECUTABLE_NAME}
    PRIVATE
    ${CORE_COMPILE_DEFS}
  )

  target_include_directories(${EXECUTABLE_NAME}
    PRIVATE
    ${GLFW_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/platforms/common
  )

  target_link_libraries(${EXECUTABLE_NAME}
    ${CORE_LIBRARY}
    glfw
  )
  set_target_properties(${EXECUTABLE_NAME}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  )
  add_resources(${EXECUTABLE_NAME} "${PROJECT_SOURCE_DIR}/scenes")

  set_target_properties(
    example minigzip zlib
    PROPERTIES EXCLUDE_FROM_ALL 1 EXCLUDE_FROM_DEFAULT_BUILD 1
  )

endif()
