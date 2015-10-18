# options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++1y")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unknown-pragmas")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wparentheses")
set(CXX_FLAGS_DEBUG "-g -O0")

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-gnu-zero-variadic-macro-arguments")
endif()

add_definitions(-DPLATFORM_LINUX)

# load core library
add_subdirectory(${PROJECT_SOURCE_DIR}/core)
include_directories(${CORE_INCLUDE_DIRS})


if(APPLICATION)

  # load glfw
  include(${PROJECT_SOURCE_DIR}/toolchains/add_glfw.cmake)

  set(EXECUTABLE_NAME "tangram")

  # add sources and include headers
  find_sources_and_include_directories(
    ${PROJECT_SOURCE_DIR}/linux/src/*.h
    ${PROJECT_SOURCE_DIR}/linux/src/*.cpp)

  add_executable(${EXECUTABLE_NAME} ${SOURCES})

  target_link_libraries(${EXECUTABLE_NAME}
    core -lcurl
    glfw ${GLFW_LIBRARIES})

  add_dependencies(${EXECUTABLE_NAME} copy_resources)

endif()
