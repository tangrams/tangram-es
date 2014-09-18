file(GLOB_RECURSE CORE_SOURCES "${PROJECT_SOURCE_DIR}/core/*.cpp")
file(GLOB_RECURSE CORE_HEADERS "${PROJECT_SOURCE_DIR}/core/*.h")

set(INCLUDE_DIRS "")
foreach(_headerFile ${CORE_HEADERS})
	get_filename_component(_dir ${_headerFile} PATH)
	list(APPEND INCLUDE_DIRS ${_dir})
endforeach()
list(REMOVE_DUPLICATES INCLUDE_DIRS)

include_directories(${INCLUDE_DIRS})

set(SOURCES
    ${SOURCES}
    ${CORE_SOURCES})
