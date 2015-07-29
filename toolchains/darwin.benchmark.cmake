# options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -stdlib=libc++ -std=c++11")

add_definitions(-DPLATFORM_OSX)

# load glfw
include(${PROJECT_SOURCE_DIR}/toolchains/add_glfw.cmake)

# load core library
add_subdirectory(${PROJECT_SOURCE_DIR}/core)
include_directories(${CORE_INCLUDE_DIRS})
include_directories(${CORE_LIBRARIES_INCLUDE_DIRS})

set(OSX_PLATFORM_SRC ${PROJECT_SOURCE_DIR}/osx/src/platform_osx.mm)

file(GLOB BENCH_SOURCES bench/*.cpp)

# create an executable per benchmark
foreach(_src_file_path ${BENCH_SOURCES})
    string(REPLACE ".cpp" "" bench ${_src_file_path})
    string(REGEX MATCH "([^/]*)$" bench_name ${bench})

    set(EXECUTABLE_NAME "${bench_name}_bench.out")

    add_executable(${EXECUTABLE_NAME} ${_src_file_path} ${OSX_PLATFORM_SRC})

    target_link_libraries(${EXECUTABLE_NAME} benchmark core glfw ${GLFW_LIBRARIES})

endforeach(_src_file_path ${BENCH_SOURCES})

# copy resources in order to make tests benchmark resources dependency
file(GLOB_RECURSE RESOURCES ${PROJECT_SOURCE_DIR}/core/resources/*)
foreach(_resource ${RESOURCES})
    file(COPY ${_resource} DESTINATION ${PROJECT_SOURCE_DIR}/build/bench/bin)
endforeach()
