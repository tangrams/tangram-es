# set for test in other cmake files
set(PLATFORM_WINDOWS ON)

# global compile options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -utf-8 -wd4514 -wd4710 -wd4996")

check_unsupported_compiler_version()

# compile definitions (adds -DPLATFORM_WINDOWS)
set(CORE_COMPILE_DEFS PLATFORM_WINDOWS)

if (USE_EXTERNAL_LIBS)
include(${EXTERNAL_LIBS_DIR}/core-dependencies.cmake)
include(${EXTERNAL_LIBS_DIR}/glfw.cmake)
else()
add_subdirectory(${PROJECT_SOURCE_DIR}/external)
endif()

# load core library
add_subdirectory(${PROJECT_SOURCE_DIR}/core)
