set(INCLUDE_DIR libtess2/Include)
set(SOURCE_DIR libtess2/Source)

file(GLOB_RECURSE FOUND_SOURCES "${SOURCE_DIR}/*.c")

include_directories(${INCLUDE_DIR})
add_library(libtess2 ${FOUND_SOURCES})