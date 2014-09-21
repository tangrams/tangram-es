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

add_library(core ${FOUND_SOURCES})
target_link_libraries(core json)
