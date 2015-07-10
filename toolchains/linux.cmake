# options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11")
set(CXX_FLAGS_DEBUG "-g -O0")
set(EXECUTABLE_NAME "tangram")

add_definitions(-DPLATFORM_LINUX)

include_directories(/usr/local/include)

# add sources and include headers
find_sources_and_include_directories(
    ${PROJECT_SOURCE_DIR}/linux/src/*.h 
    ${PROJECT_SOURCE_DIR}/linux/src/*.cpp)

# configure glfw
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Build the GLFW example programs")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "Build the GLFW test programs")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "Build the GLFW documentation")
set(GLFW_INSTALL OFF CACHE BOOL "Generate installation target")
add_subdirectory(${PROJECT_SOURCE_DIR}/glfw)
include_directories(${PROJECT_SOURCE_DIR}/glfw/include)

# load core library
add_subdirectory(${PROJECT_SOURCE_DIR}/core)
include_directories(${CORE_INCLUDE_DIRS})

# link and build functions
function(link_libraries)

    target_link_libraries(${EXECUTABLE_NAME} core -lcurl glfw ${GLFW_LIBRARIES})

endfunction()

function(build) 

    add_executable(${EXECUTABLE_NAME} ${SOURCES})

    file(GLOB_RECURSE RESOURCES ${PROJECT_SOURCE_DIR}/core/resources/*)
    foreach(_resource ${RESOURCES})
        file(COPY ${_resource} DESTINATION ${PROJECT_SOURCE_DIR}/build/linux/bin)
    endforeach()

endfunction()
