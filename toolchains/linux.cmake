# set for test in other cmake files
set(PLATFORM_LINUX ON)

# global compile options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wignored-qualifiers -Wtype-limits -Wmissing-field-initializers")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer")

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-gnu-zero-variadic-macro-arguments")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}  -lc++ -lc++abi")
endif()

if (CMAKE_COMPILER_IS_GNUCC)
  execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion
    OUTPUT_VARIABLE GCC_VERSION)
  string(REGEX MATCHALL "[0-9]+" GCC_VERSION_COMPONENTS ${GCC_VERSION})
  list(GET GCC_VERSION_COMPONENTS 0 GCC_MAJOR)
  list(GET GCC_VERSION_COMPONENTS 1 GCC_MINOR)

  message(STATUS "Using gcc ${GCC_VERSION}")
  if (GCC_VERSION VERSION_GREATER 5.1)
    message(STATUS "USE CXX11_ABI")
    add_definitions("-D_GLIBCXX_USE_CXX11_ABI=1")
  endif()
endif()

check_unsupported_compiler_version()

# compile definitions (adds -DPLATFORM_LINUX)
set(CORE_COMPILE_DEFS PLATFORM_LINUX)

# load core library
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
  find_sources_and_include_directories(
    ${PROJECT_SOURCE_DIR}/platforms/linux/src/*.h
    ${PROJECT_SOURCE_DIR}/platforms/linux/src/*.cpp)

  add_executable(${EXECUTABLE_NAME}
    ${SOURCES}
    ${PROJECT_SOURCE_DIR}/platforms/common/platform_gl.cpp
    ${PROJECT_SOURCE_DIR}/platforms/common/urlClient.cpp
    ${PROJECT_SOURCE_DIR}/platforms/common/glfwApp.cpp
    )

  target_include_directories(${EXECUTABLE_NAME}
    PUBLIC
    ${GLFW_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/platforms/common)

  target_link_libraries(${EXECUTABLE_NAME}
    ${CORE_LIBRARY}
    -lcurl glfw
    # only used when not using external lib
    -ldl
    -pthread
    ${GLFW_LIBRARIES}
    ${OPENGL_LIBRARIES})

  add_resources(${EXECUTABLE_NAME} "${PROJECT_SOURCE_DIR}/scenes")

endif()
