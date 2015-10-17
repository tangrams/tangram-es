# Defines the following cached variables
#   - CORE_INCLUDE_DIRS, the path where to CORE headers are located
#   - CORE_LIBRARIES_INCLUDE_DIRS, the path to CORE libraries headers

project(core)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # using regular Clang or AppleClang
    list(APPEND CORE_CXX_FLAGS -Wno-gnu-anonymous-struct)
    list(APPEND CORE_CXX_FLAGS -Wno-nested-anon-types)
    list(APPEND CORE_CXX_FLAGS -Wno-gnu-zero-variadic-macro-arguments)
endif()

set(INCLUDE_DIR include)
set(DEPENDENCIES_DIR dependencies)
set(SOURCE_DIR src)

add_definitions(-DLOG_LEVEL=2)

add_subdirectory("${DEPENDENCIES_DIR}")

file(GLOB_RECURSE FOUND_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_DIR}/*.cpp")

set(INCLUDE_DIRS "")
list(APPEND INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_DIR}")

set(CORE_INCLUDE_DIRS ${INCLUDE_DIRS} CACHE INTERNAL "core include directories" FORCE)
list(APPEND INCLUDE_DIRS ${INCLUDE_DIR})
list(APPEND INCLUDE_DIRS ${INCLUDE_DIR}/mapbox)
list(APPEND INCLUDE_DIRS ${INCLUDE_DIR}/stb)
list(APPEND INCLUDE_DIRS ${INCLUDE_DIR}/glm)
list(APPEND INCLUDE_DIRS ${INCLUDE_DIR}/isect2d/include)
list(APPEND INCLUDE_DIRS ${INCLUDE_DIR}/fontstash-es/fontstash)
list(REMOVE_DUPLICATES INCLUDE_DIRS)

set(CORE_LIBRARIES_INCLUDE_DIRS
    ${PROJECT_SOURCE_DIR}/${INCLUDE_DIR}
    ${PROJECT_SOURCE_DIR}/${INCLUDE_DIR}/glm
    ${PROJECT_SOURCE_DIR}/${INCLUDE_DIR}/stb
    ${PROJECT_SOURCE_DIR}/${INCLUDE_DIR}/fontstash-es/fontstash
    ${PROJECT_SOURCE_DIR}/${INCLUDE_DIR}/pbf
    ${PROJECT_SOURCE_DIR}/${INCLUDE_DIR}/rapidjson
    ${PROJECT_SOURCE_DIR}/${INCLUDE_DIR}/isect2d/include
    CACHE INTERNAL "core libraries include directories" FORCE)

#if(NOT DEFINED CORE_LIB_NAME)
set(CORE_LIB_NAME core)
#endif()

add_library(${CORE_LIB_NAME} ${FOUND_SOURCES})

target_link_libraries(${CORE_LIB_NAME}
  PUBLIC
  duktape
  css-color-parser-cpp
  geojson-vt-cpp
  yaml-cpp)

target_include_directories(${CORE_LIB_NAME}
  PUBLIC
  ${INCLUDE_DIRS})

target_compile_options(${CORE_LIB_NAME}
  PUBLIC
  ${CORE_CXX_FLAGS})

target_compile_definitions(${CORE_LIB_NAME}
  PUBLIC
  ${CORE_COMPILE_DEFS})

# make groups for xcode
group_recursive_sources(src "src")

# post build commands
add_custom_command(TARGET ${CORE_LIB_NAME}
        POST_BUILD
        COMMAND ${CORE_POSTBUILD_COMMANDS}
        COMMENT ${CORE_COMMAND_MSG} VERBATIM)

### Resources
file(GLOB_RECURSE RESOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/resources/*")

add_custom_target(copy_resources
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/resources
    ${CMAKE_BINARY_DIR}/bin
  # Make resources show up in IDE, at least this should do it
  SOURCES ${RESOURCE_FILES})
