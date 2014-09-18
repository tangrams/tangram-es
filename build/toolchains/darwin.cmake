set(INCLUDE_DIR ${PROJECT_SOURCE_DIR}/core/include/)

# wip (should be fixed with glew)
add_definitions(-DPLATFORM_OSX=1)

include_directories(${INCLUDE_DIR})
add_library(json "${INCLUDE_DIR}/jsoncpp.cpp")

add_subdirectory("${PROJECT_SOURCE_DIR}/osx/lib/glfw3")
include_directories("${PROJECT_SOURCE_DIR}/osx/lib/glfw3/include")

include(${PROJECT_SOURCE_DIR}/build/toolchains/core.cmake)

find_package(OpenGL REQUIRED)

if(OPENGL_FOUND)
    message("OpenGL correctly found in ${OPENGL_INCLUDE_DIR}")
    include_directories(${OPENGL_INCLUDE_DIR})
else(OPENGL_FOUND)
    message("OpenGL environment missing")
    return()
endif(OPENGL_FOUND)

include_directories(${OpenGL_INCLUDE_DIRS})

find_package(curl REQUIRED)

if(CURL_FOUND) 
    message("Curl library found")
else(CURL_FOUND)
    message("Curl library not found")
endif(CURL_FOUND)

function(link_libraries)
    target_link_libraries(${EXECUTABLE_NAME} glfw ${GLFW_LIBRARIES})
    target_link_libraries(${EXECUTABLE_NAME} ${OPENGL_LIBRARIES})
    target_link_libraries(${EXECUTABLE_NAME} json)
    target_link_libraries(${EXECUTABLE_NAME} curl ${CURL_LIBRARIES})
endfunction()

# WIP should look to more sources than only main.cpp
set(SOURCES
    ${SOURCES}
    ${PROJECT_SOURCE_DIR}/osx/src/main.cpp)
