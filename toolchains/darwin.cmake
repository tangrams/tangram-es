# set for test in other cmake files
set(PLATFORM_OSX ON)

check_unsupported_compiler_version()

# options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -std=c++1y")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wreturn-type -Wsign-compare -Wignored-qualifiers -Wtype-limits -Wmissing-field-initializers")
set(CXX_FLAGS_DEBUG "-g -O0")

# compile definitions (adds -DPLATFORM_OSX)
set(CORE_COMPILE_DEFS PLATFORM_OSX)

# Build core library with dependencies.
add_subdirectory(${PROJECT_SOURCE_DIR}/core)

if(APPLICATION)

  set(EXECUTABLE_NAME "tangram")

  find_package(OpenGL REQUIRED)

  # Build GLFW.
  if (USE_SYSTEM_GLFW_LIBS)
    include(FindPkgConfig)
    pkg_check_modules(GLFW REQUIRED glfw3)
  else()
    # configure GLFW to build only the library
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Build the GLFW example programs")
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "Build the GLFW test programs")
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "Build the GLFW documentation")
    set(GLFW_INSTALL OFF CACHE BOOL "Generate installation target")
    add_subdirectory(${PROJECT_SOURCE_DIR}/platforms/common/glfw)
  endif()

  # add sources and include headers
  set(OSX_EXTENSIONS_FILES *.mm *.cpp)
  foreach(_ext ${OSX_EXTENSIONS_FILES})
    find_sources_and_include_directories(
      ${PROJECT_SOURCE_DIR}/platforms/osx/src/*.h
      ${PROJECT_SOURCE_DIR}/platforms/osx/src/${_ext})
  endforeach()

  add_bundle_resources(RESOURCES "${PROJECT_SOURCE_DIR}/scenes" "Resources")

  set(SOURCES ${SOURCES} ${PROJECT_SOURCE_DIR}/platforms/common/platform_gl.cpp ${PROJECT_SOURCE_DIR}/platforms/common/glfwApp.cpp)

  add_executable(${EXECUTABLE_NAME} MACOSX_BUNDLE ${SOURCES} ${RESOURCES} ${OSX_RESOURCES})

  target_include_directories(${EXECUTABLE_NAME}
    PUBLIC
    ${GLFW_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/platforms/common)

  target_link_libraries(${EXECUTABLE_NAME}
    ${CORE_LIBRARY}
    glfw
    ${GLFW_LIBRARIES}
    ${OPENGL_LIBRARIES})

  # add resource files and property list
  set_target_properties(${EXECUTABLE_NAME} PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST "${PROJECT_SOURCE_DIR}/platforms/osx/Info.plist"
    RESOURCE "${OSX_RESOURCES}")

endif()
