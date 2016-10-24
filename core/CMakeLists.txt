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
  add_definitions(-DDEBUG)
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
list(APPEND INCLUDE_DIRS ${INCLUDE_DIR}/shaders)
list(APPEND INCLUDE_DIRS ${INCLUDE_DIR}/isect2d/include)
list(APPEND INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/api")
list(REMOVE_DUPLICATES INCLUDE_DIRS)

set(CORE_LIBRARIES_INCLUDE_DIRS
    ${PROJECT_SOURCE_DIR}/${INCLUDE_DIR}
    ${PROJECT_SOURCE_DIR}/${INCLUDE_DIR}/shaders
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

if (PLATFORM_TARGET STREQUAL "linux" OR PLATFORM_TARGET STREQUAL "windows")
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/api)
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/demo)
endif()

# make groups for xcode
group_recursive_sources(src "src")
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(CMD_SCRIPT ${PROJECT_SOURCE_DIR}/../scripts/incbin.bat)
else()
  set(CMD_SCRIPT "bash" ${PROJECT_SOURCE_DIR}/../scripts/incbin.bat)
endif()

# add compile shader targets
file(GLOB_RECURSE SHADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*")
set(SHADER_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/shaders)
foreach(_shader ${SHADER_FILES})
    get_filename_component(_shader_name ${_shader} NAME_WE)
    get_filename_component(_shader_ext ${_shader} EXT)
    string(REGEX REPLACE "\\." "" _shader_ext ${_shader_ext})
    if(${_shader_ext} STREQUAL "fs" OR ${_shader_ext} STREQUAL "glsl" OR ${_shader_ext} STREQUAL "vs")
        set(_shader_name ${_shader_name}_${_shader_ext})
        set(SHADER_OUTPUT ${SHADER_OUTPUT_DIRECTORY}/${_shader_name}.h)
        add_custom_target(
            ${_shader_name}
            DEPENDS ${SHADER_OUTPUT})
        add_custom_command(
            OUTPUT ${SHADER_OUTPUT}
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/..
            MAIN_DEPENDENCY ${_shader}
            COMMAND ${CMD_SCRIPT}
                ${_shader}
                core/include/shaders/${_shader_name}.h
                ${_shader_name})
        add_dependencies(${CORE_LIBRARY} ${_shader_name})
    endif()
endforeach()

# add target to transform the default point texture into a data header
set(DEFAULT_POINT_TEXTURE_IMAGE ${CMAKE_CURRENT_SOURCE_DIR}/../images/default-point.png)
set(DEFAULT_POINT_TEXTURE_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/include/defaultPointTextureData.h)
add_custom_target(default_point_texture DEPENDS ${DEFAULT_POINT_TEXTURE_HEADER})
add_custom_command(
  OUTPUT ${DEFAULT_POINT_TEXTURE_HEADER}
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/..
  MAIN_DEPENDENCY ${DEFAULT_POINT_TEXTURE_IMAGE}
  COMMAND ${CMD_SCRIPT} ${DEFAULT_POINT_TEXTURE_IMAGE} ${DEFAULT_POINT_TEXTURE_HEADER} default_point_texture
  )
add_dependencies(${CORE_LIBRARY} default_point_texture)
