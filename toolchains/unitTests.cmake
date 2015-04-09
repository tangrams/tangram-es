# options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -stdlib=libc++ -std=c++11")

add_definitions(-DPLATFORM_OSX) 

# include headers for homebrew-installed libraries
include_directories(/usr/local/include)

# load core library
add_subdirectory(${PROJECT_SOURCE_DIR}/core)
include_directories(${CORE_INCLUDE_DIRS})
include_directories(${CORE_LIBRARIES_INCLUDE_DIRS})

set(OSX_PLATFORM_SRC ${PROJECT_SOURCE_DIR}/osx/src/platform_osx.mm)

find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)

list(APPEND GLFW_LDFLAGS
        "-framework OpenGL" 
        "-framework Cocoa" 
        "-framework IOKit" 
        "-framework CoreFoundation"   
        "-framework CoreVideo")

file(GLOB TEST_SOURCES tests/unit/*.cpp)

# create an executable per test
foreach(_src_file_path ${TEST_SOURCES})
    string(REPLACE ".cpp" "" test_case ${_src_file_path})
    string(REGEX MATCH "([^/]*)$" test_name ${test_case})
    
    set(EXECUTABLE_NAME "${test_name}.out")

    add_executable(${EXECUTABLE_NAME} ${_src_file_path} ${OSX_PLATFORM_SRC})

    target_link_libraries(${EXECUTABLE_NAME} -lcurl)
    target_link_libraries(${EXECUTABLE_NAME} core ${GLFW_LDFLAGS})
endforeach(_src_file_path ${TEST_SOURCES})

# copy resources in order to make tests with resources dependency
file(GLOB_RECURSE RESOURCES ${PROJECT_SOURCE_DIR}/core/resources/*)
foreach(_resource ${RESOURCES})
    file(COPY ${_resource} DESTINATION ${PROJECT_SOURCE_DIR}/build/tests/unit/bin)
endforeach()
