check_unsupported_compiler_version()

# options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -std=c++1y")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wreturn-type -Wsign-compare -Wignored-qualifiers -Wtype-limits -Wmissing-field-initializers")
set(CXX_FLAGS_DEBUG "-g -O0")

add_definitions(-DTANGRAM_OSX)

# Build core library with dependencies.
add_subdirectory(${PROJECT_SOURCE_DIR}/core)

if(TANGRAM_APPLICATION)

  set(EXECUTABLE_NAME "tangram")

  get_mapzen_api_key(MAPZEN_API_KEY)
  add_definitions(-DMAPZEN_API_KEY="${MAPZEN_API_KEY}")

  if($ENV{CIRCLE_BUILD_NUM})
    add_definitions(-DBUILD_NUM_STRING="\($ENV{CIRCLE_BUILD_NUM}\)")
  endif()

  find_package(OpenGL REQUIRED)

  # Build GLFW.
  if (TANGRAM_USE_SYSTEM_GLFW_LIBS)
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

  add_bundle_resources(RESOURCES "${PROJECT_SOURCE_DIR}/scenes" "Resources")

  set(SOURCES
    ${PROJECT_SOURCE_DIR}/platforms/common/platform_gl.cpp
    ${PROJECT_SOURCE_DIR}/platforms/common/glfwApp.cpp
    ${PROJECT_SOURCE_DIR}/platforms/osx/src/main.mm
    ${PROJECT_SOURCE_DIR}/platforms/osx/src/osxPlatform.mm
    ${PROJECT_SOURCE_DIR}/platforms/common/TGFontConverter.mm
    )

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
