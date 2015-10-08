# Defines the following cached variables
#   - CORE_INCLUDE_DIRS, the path where to CORE headers are located
#   - CORE_LIBRARIES_INCLUDE_DIRS, the path to CORE libraries headers
#   - CORE_LIBRARY, the core library name

cmake_minimum_required(VERSION 2.8)
project(core)

set(INCLUDE_DIR include)
set(DEPENDENCIES_DIR dependencies)
set(SOURCE_DIR src)
set(CORE_LIBRARY core CACHE INTERNAL "core library name" FORCE)

add_definitions(-DLOG_LEVEL=2)

add_subdirectory("${DEPENDENCIES_DIR}")

file(GLOB_RECURSE FOUND_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_DIR}/*.cpp")

set(INCLUDE_DIRS "")
list(APPEND INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_DIR}")

set(CORE_INCLUDE_DIRS ${INCLUDE_DIRS} CACHE INTERNAL "core include directories" FORCE)
list(APPEND INCLUDE_DIRS "${INCLUDE_DIR}")
list(APPEND INCLUDE_DIRS "${INCLUDE_DIR}/mapbox")
list(APPEND INCLUDE_DIRS "${INCLUDE_DIR}/stb")
list(APPEND INCLUDE_DIRS "${INCLUDE_DIR}/glm")
list(APPEND INCLUDE_DIRS "${INCLUDE_DIR}/isect2d/include")
list(APPEND INCLUDE_DIRS "${INCLUDE_DIR}/fontstash-es/fontstash")
list(REMOVE_DUPLICATES INCLUDE_DIRS)

set(CORE_LIBRARIES_INCLUDE_DIRS
    ${PROJECT_SOURCE_DIR}/${INCLUDE_DIR}
    ${PROJECT_SOURCE_DIR}/${INCLUDE_DIR}/glm
    ${PROJECT_SOURCE_DIR}/${INCLUDE_DIR}/stb
    ${PROJECT_SOURCE_DIR}/${INCLUDE_DIR}/catch
    ${PROJECT_SOURCE_DIR}/${INCLUDE_DIR}/fontstash-es/fontstash
    ${PROJECT_SOURCE_DIR}/${INCLUDE_DIR}/pbf
    ${PROJECT_SOURCE_DIR}/${INCLUDE_DIR}/rapidjson
    ${PROJECT_SOURCE_DIR}/${INCLUDE_DIR}/isect2d/include
    CACHE INTERNAL "core libraries include directories" FORCE)

# adding some other dependent target files if needed
set(FOUND_SOURCES ${FOUND_SOURCES} ${ADDITIONAL_TARGET_DEPENDENT_SRC_FILES})

add_library(${CORE_LIBRARY} ${CORE_LIB_TYPE} ${FOUND_SOURCES})
target_link_libraries(${CORE_LIBRARY} duktape css-color-parser-cpp yaml-cpp geojson-vt-cpp ${CORE_LIB_DEPS})

target_include_directories(${CORE_LIBRARY}
  PRIVATE
  ${INCLUDE_DIRS}
  PUBLIC
  ${CORE_INCLUDE_DIRS})

# make groups for xcode
group_recursive_sources(src "src")

