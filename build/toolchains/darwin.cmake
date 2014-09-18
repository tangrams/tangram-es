set(INCLUDE_DIR ${PROJECT_SOURCE_DIR}/core/include/)

add_definitions(-DPLATFORM_OSX=1)

add_library(json "${INCLUDE_DIR}/jsoncpp.cpp")

# build glfw files
add_subdirectory("${PROJECT_SOURCE_DIR}/osx/lib/glfw3")

include_directories(${INCLUDE_DIR})
include_directories("${PROJECT_SOURCE_DIR}/osx/lib/glfw3/include")

find_sources_and_include_directories(
    ${PROJECT_SOURCE_DIR}/core/*.h 
    ${PROJECT_SOURCE_DIR}/core/*.cpp)

find_sources_and_include_directories(
    ${PROJECT_SOURCE_DIR}/osx/*.h 
    ${PROJECT_SOURCE_DIR}/osx/*.cpp)

function(link_libraries)
    check_and_link_libraries(${EXECUTABLE_NAME} curl OpenGL)
    target_link_libraries(${EXECUTABLE_NAME} glfw ${GLFW_LIBRARIES})
    target_link_libraries(${EXECUTABLE_NAME} json)
endfunction()
