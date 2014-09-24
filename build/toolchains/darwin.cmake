# options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -stdlib=libc++ -std=c++0x")
set(EXECUTABLE_NAME "tangram.out")

add_definitions(-DPLATFORM_OSX=1)

# load glfw3 library
add_subdirectory(${PROJECT_SOURCE_DIR}/osx/lib/glfw3)
include_directories(${PROJECT_SOURCE_DIR}/osx/lib/glfw3/include)

# load core library
include_directories(${PROJECT_SOURCE_DIR}/core/include/)
add_subdirectory(${PROJECT_SOURCE_DIR}/core)
include_recursive_dirs(${PROJECT_SOURCE_DIR}/core/*.h)

# add sources and include headers
find_sources_and_include_directories(
    ${PROJECT_SOURCE_DIR}/osx/*.h 
    ${PROJECT_SOURCE_DIR}/osx/*.cpp)

# link and build functions
function(link_libraries)
    check_and_link_libraries(${EXECUTABLE_NAME} curl OpenGL)
    target_link_libraries(${EXECUTABLE_NAME} glfw ${GLFW_LIBRARIES})
    target_link_libraries(${EXECUTABLE_NAME} core)
endfunction()

function(build) 
    add_executable(${EXECUTABLE_NAME} ${SOURCES})
endfunction()
