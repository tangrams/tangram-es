# Defines the following cached variables
#   - CORE_INCLUDE_DIRS, the path where to CORE headers are located
#   - CORE_LIBRARIES_INCLUDE_DIRS, the path to CORE libraries headers
#   - CORE_LIBRARY, the core library name

project(core)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # using regular Clang or AppleClang
    list(APPEND CORE_CXX_FLAGS -Wno-gnu-anonymous-struct)
    list(APPEND CORE_CXX_FLAGS -Wno-nested-anon-types)
    list(APPEND CORE_CXX_FLAGS -Wno-gnu-zero-variadic-macro-arguments)
endif()

set(INCLUDE_DIR include)
set(SOURCE_DIR src)
set(CORE_LIBRARY core CACHE INTERNAL "core library name" FORCE)


if (CMAKE_BUILD_TYPE MATCHES Debug)
  add_definitions(-DLOG_LEVEL=3)
else()
  add_definitions(-DLOG_LEVEL=2)
endif()

if (TANGRAM_WARN_ON_RULE_CONFLICT)
  add_definitions(-DTANGRAM_WARN_ON_RULE_CONFLICT)
endif()

file(GLOB_RECURSE FOUND_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_DIR}/*.cpp")
file(GLOB_RECURSE FOUND_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_DIR}/*.h")

set(INCLUDE_DIRS "")
list(APPEND INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_DIR}")

set(CORE_INCLUDE_DIRS ${INCLUDE_DIRS} CACHE INTERNAL "core include directories" FORCE)
list(APPEND INCLUDE_DIRS ${INCLUDE_DIR})
list(APPEND INCLUDE_DIRS ${INCLUDE_DIR}/mapbox)
list(APPEND INCLUDE_DIRS ${INCLUDE_DIR}/sdf)
list(APPEND INCLUDE_DIRS ${INCLUDE_DIR}/stb)
list(APPEND INCLUDE_DIRS ${INCLUDE_DIR}/glm)
list(APPEND INCLUDE_DIRS ${INCLUDE_DIR}/isect2d/include)
list(REMOVE_DUPLICATES INCLUDE_DIRS)

set(CORE_LIBRARIES_INCLUDE_DIRS
    ${PROJECT_SOURCE_DIR}/${INCLUDE_DIR}
    ${PROJECT_SOURCE_DIR}/${INCLUDE_DIR}/glm
    ${PROJECT_SOURCE_DIR}/${INCLUDE_DIR}/sdf
    ${PROJECT_SOURCE_DIR}/${INCLUDE_DIR}/stb
    ${PROJECT_SOURCE_DIR}/${INCLUDE_DIR}/pbf
    ${PROJECT_SOURCE_DIR}/${INCLUDE_DIR}/rapidjson
    ${PROJECT_SOURCE_DIR}/${INCLUDE_DIR}/isect2d/include
    CACHE INTERNAL "core libraries include directories" FORCE)

#include( ${PROJECT_SOURCE_DIR}/../../alfons/build/alfons-exports.cmake REQUIRED )
#include( ${PROJECT_SOURCE_DIR}/../../alfons/debug/alfons-exports.cmake REQUIRED )

add_library(${CORE_LIBRARY} ${FOUND_SOURCES} ${FOUND_HEADERS})

target_link_libraries(${CORE_LIBRARY}
  PUBLIC
  duktape
  css-color-parser-cpp
  geojson-vt-cpp
  yaml-cpp
  alfons)

target_include_directories(${CORE_LIBRARY}
  PUBLIC
  ${INCLUDE_DIRS})

target_compile_options(${CORE_LIBRARY}
  PUBLIC
  ${CORE_CXX_FLAGS})

target_compile_definitions(${CORE_LIBRARY}
  PUBLIC
  ${CORE_COMPILE_DEFS})

# make groups for xcode
group_recursive_sources(src "src")

### Resources
file(GLOB_RECURSE RESOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/resources/*")

add_custom_target(copy_resources
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/resources
    ${CMAKE_BINARY_DIR}/bin
  # Make resources show up in IDE, at least this should do it
  SOURCES ${RESOURCE_FILES})
