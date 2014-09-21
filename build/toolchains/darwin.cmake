set(INCLUDE_DIR ${PROJECT_SOURCE_DIR}/core/include/)

add_definitions(-DPLATFORM_OSX=1)

add_subdirectory("${PROJECT_SOURCE_DIR}/osx/lib/glfw3")
set(INCLUDE_DIR
    ${INCLUDE_DIR}
    "${PROJECT_SOURCE_DIR}/osx/lib/glfw3/include")

include_directories(${INCLUDE_DIR})

add_subdirectory("${PROJECT_SOURCE_DIR}/core")

#find_sources_and_include_directories(
#    ${PROJECT_SOURCE_DIR}/core/*.h 
#    ${PROJECT_SOURCE_DIR}/core/*.cpp)

file(GLOB_RECURSE FOUND_HEADERS ${PROJECT_SOURCE_DIR}/core/*.h)

set(INCLUDE_DIRS "")
foreach(_headerFile ${FOUND_HEADERS})
    get_filename_component(_dir ${_headerFile} PATH)
    list(APPEND INCLUDE_DIRS ${_dir})
endforeach()
list(REMOVE_DUPLICATES INCLUDE_DIRS)

include_directories(${INCLUDE_DIRS})

find_sources_and_include_directories(
    ${PROJECT_SOURCE_DIR}/osx/*.h 
    ${PROJECT_SOURCE_DIR}/osx/*.cpp)

function(link_libraries)
    check_and_link_libraries(${EXECUTABLE_NAME} curl OpenGL)
    target_link_libraries(${EXECUTABLE_NAME} glfw ${GLFW_LIBRARIES})
    #target_link_libraries(${EXECUTABLE_NAME} json)
    target_link_libraries(${EXECUTABLE_NAME} core)
endfunction()
