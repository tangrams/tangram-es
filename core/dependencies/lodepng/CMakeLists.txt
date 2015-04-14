set(INCLUDE_DIR include)
set(SOURCE_DIR src)

file(GLOB_RECURSE FOUND_SOURCES ${SOURCE_DIR}/*.cpp)

include_directories(${INCLUDE_DIR})
add_library(lodepng ${FOUND_SOURCES})