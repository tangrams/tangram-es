# options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++1y")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unknown-pragmas")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wparentheses")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wuninitialized")
set(CXX_FLAGS_DEBUG "-g -O0")

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-gnu-zero-variadic-macro-arguments")
endif()

# if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR
#     "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
#   # using Clang
# elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
#   # using GCC
# endif()

set(EXECUTABLE_NAME "tangram")

add_definitions(-DPLATFORM_LINUX)

# load core library
add_subdirectory(${PROJECT_SOURCE_DIR}/core)
include_directories(${CORE_INCLUDE_DIRS})

# add sources and include headers
find_sources_and_include_directories(
    ${PROJECT_SOURCE_DIR}/linux/src/*.h
    ${PROJECT_SOURCE_DIR}/linux/src/*.cpp)

# load glfw
include(${PROJECT_SOURCE_DIR}/toolchains/add_glfw.cmake)

# link and build functions
function(link_libraries)

    target_link_libraries(${EXECUTABLE_NAME} ${CORE_LIBRARY} -lcurl glfw ${GLFW_LIBRARIES})

endfunction()

function(build)

    add_executable(${EXECUTABLE_NAME} ${SOURCES})

    add_custom_command(TARGET tangram POST_BUILD COMMAND
      COMMAND ${CMAKE_COMMAND} -E echo "Copying data..."
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/core/resources $<TARGET_FILE_DIR:tangram>
    )

endfunction()
