# options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11")
set(CXX_FLAGS_DEBUG "-g -O0")
set(EXECUTABLE_NAME "tangram.out")

add_definitions(-DPLATFORM_OSX)

find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)
include_directories(${GLFW_INCLUDE_DIRS})

# load core library
include_directories(${PROJECT_SOURCE_DIR}/core/include/)
add_subdirectory(${PROJECT_SOURCE_DIR}/core)
include_recursive_dirs(${PROJECT_SOURCE_DIR}/core/*.h)

# add sources and include headers
find_sources_and_include_directories(
    ${PROJECT_SOURCE_DIR}/osx/src/*.h 
    ${PROJECT_SOURCE_DIR}/osx/src/*.cpp)

include_directories("${PROJECT_SOURCE_DIR}/osx/include/")

# link and build functions
function(link_libraries)

    target_link_libraries(${EXECUTABLE_NAME} ${GLFW_STATIC_LIBRARIES} core)

endfunction()

function(build) 

    add_executable(${EXECUTABLE_NAME} ${SOURCES})

endfunction()
