# set for test in other cmake files
set(PLATFORM_LINUX ON)

# global compile options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++1y")

if (USE_EXTERNAL_LIBS)
include(${EXTERNAL_LIBS_DIR}/yaml-cpp.cmake)
include(${EXTERNAL_LIBS_DIR}/glfw.cmake)
else()
add_subdirectory(${PROJECT_SOURCE_DIR}/external)
endif()

# if(CMAKE_COMPILER_IS_GNUCXX)
#   list(APPEND CORE_CXX_FLAGS -ffast-math)
# endif()

# compile definitions (adds -DPLATFORM_LINUX)
set(CORE_COMPILE_DEFS PLATFORM_LINUX)

# load core library
add_subdirectory(${PROJECT_SOURCE_DIR}/core)

if(APPLICATION)

  set(EXECUTABLE_NAME "tangram")

  find_package(OpenGL REQUIRED)

  # add sources and include headers
  find_sources_and_include_directories(
    ${PROJECT_SOURCE_DIR}/linux/src/*.h
    ${PROJECT_SOURCE_DIR}/linux/src/*.cpp)

  add_executable(${EXECUTABLE_NAME} ${SOURCES})

  target_link_libraries(${EXECUTABLE_NAME}
    core -lcurl glfw
    # only used when not using external lib
    -ldl
    ${GLFW_LIBRARIES}
    ${OPENGL_LIBRARIES})

  add_dependencies(${EXECUTABLE_NAME} copy_resources)

endif()
