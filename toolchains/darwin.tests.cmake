# options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -stdlib=libc++ -std=c++1y")

add_definitions(-DPLATFORM_OSX)

# load glfw
include(${PROJECT_SOURCE_DIR}/toolchains/add_glfw.cmake)

# load core library
add_subdirectory(${PROJECT_SOURCE_DIR}/core)
include_directories(${CORE_INCLUDE_DIRS})
include_directories(${CORE_LIBRARIES_INCLUDE_DIRS})

set(OSX_PLATFORM_SRC ${PROJECT_SOURCE_DIR}/osx/src/platform_osx.mm)

file(GLOB TEST_SOURCES tests/unit/*.cpp)

set(LAST_TARGET "")

# create an executable per test
foreach(_src_file_path ${TEST_SOURCES})
    string(REPLACE ".cpp" "" test_case ${_src_file_path})
    string(REGEX MATCH "([^/]*)$" test_name ${test_case})

    set(EXECUTABLE_NAME "${test_name}.out")

    add_executable(${EXECUTABLE_NAME} ${_src_file_path} ${OSX_PLATFORM_SRC})

    target_link_libraries(${EXECUTABLE_NAME} core glfw ${GLFW_LIBRARIES})

    set(LAST_TARGET ${EXECUTABLE_NAME})

endforeach(_src_file_path ${TEST_SOURCES})

# copy resources in order to make tests with resources dependency
add_custom_command(TARGET ${LAST_TARGET} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory
                   ${PROJECT_SOURCE_DIR}/core/resources $<TARGET_FILE_DIR:${LAST_TARGET}>)
