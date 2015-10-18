# options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -stdlib=libc++ -std=c++1y")
set(CXX_FLAGS_DEBUG "-g -O0")

# For CMake 3.0
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # using regular Clang or AppleClang
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-gnu-anonymous-struct")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-nested-anon-types")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-gnu-zero-variadic-macro-arguments")
endif()

add_definitions(-DPLATFORM_OSX)

# load core library
add_subdirectory(${PROJECT_SOURCE_DIR}/core)
include_directories(${CORE_INCLUDE_DIRS})


if(APPLICATION)

  # load glfw
  include(${PROJECT_SOURCE_DIR}/toolchains/add_glfw.cmake)

  set(EXECUTABLE_NAME "tangram")

  # add sources and include headers
  set(OSX_EXTENSIONS_FILES *.mm *.cpp)
  foreach(_ext ${OSX_EXTENSIONS_FILES})
    find_sources_and_include_directories(
      ${PROJECT_SOURCE_DIR}/osx/src/*.h
      ${PROJECT_SOURCE_DIR}/osx/src/${_ext})
  endforeach()

  add_bundle_resources(RESOURCES "${PROJECT_SOURCE_DIR}/core/resources" "Resources")

  file(GLOB_RECURSE OSX_RESOURCES "${PROJECT_SOURCE_DIR}/osx/resources/**")
  string(REGEX REPLACE "[.]DS_Store" "" OSX_RESOURCES "${OSX_RESOURCES}")

  add_executable(${EXECUTABLE_NAME} MACOSX_BUNDLE ${SOURCES} ${RESOURCES} ${OSX_RESOURCES})

  target_link_libraries(${EXECUTABLE_NAME} core glfw ${GLFW_LIBRARIES})

  # add resource files and property list
  set_target_properties(${EXECUTABLE_NAME} PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST "${PROJECT_SOURCE_DIR}/osx/resources/tangram-Info.plist"
    RESOURCE "${OSX_RESOURCES}")

endif()
