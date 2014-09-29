set(INCLUDE_DIR include)
set(SOURCE_DIR src)

add_subdirectory("${INCLUDE_DIR}/json")
add_subdirectory("${INCLUDE_DIR}/glm")

file(GLOB_RECURSE FOUND_HEADERS "${SOURCE_DIR}/*.h")
file(GLOB_RECURSE FOUND_SOURCES "${SOURCE_DIR}/*.cpp")

set(INCLUDE_DIRS "")
foreach(_headerFile ${FOUND_HEADERS})
    get_filename_component(_dir ${_headerFile} PATH)
    list(APPEND INCLUDE_DIRS ${_dir})
endforeach()
list(REMOVE_DUPLICATES INCLUDE_DIRS)

include_directories(${INCLUDE_DIR}/json)
include_directories(${INCLUDE_DIRS})

add_library(core ${CORE_LIB_TYPE} ${FOUND_SOURCES})

target_link_libraries(core json ${CORE_LIB_DEPS})

# wip (should use the same library for ios/osx)
if(${VARPLATFORM} EQUAL "ios")
    target_link_libraries(core ${CMAKE_SOURCE_DIR}/ios/precompiled/libcurl.a)
endif()
