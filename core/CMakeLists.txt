# Defines the following cached variables
#   - CORE_INCLUDE_DIRS, the path where to CORE headers are located
#   - CORE_LIBRARY, the core library name

project(core)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # using regular Clang or AppleClang
    list(APPEND CORE_CXX_FLAGS -Wno-gnu-anonymous-struct)
    list(APPEND CORE_CXX_FLAGS -Wno-nested-anon-types)
    list(APPEND CORE_CXX_FLAGS -Wno-gnu-zero-variadic-macro-arguments)
endif()

set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/tangram)
set(DEPS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps)
set(GENERATED_DIR ${CMAKE_CURRENT_SOURCE_DIR}/generated)

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

file(GLOB_RECURSE FOUND_SOURCES "${SOURCE_DIR}/*.cpp")
file(GLOB_RECURSE FOUND_HEADERS "${SOURCE_DIR}/*.h")

set(CORE_INCLUDE_DIRS
  ${SOURCE_DIR}
  ${INCLUDE_DIR}
  ${GENERATED_DIR}
  ${DEPS_DIR}
  ${DEPS_DIR}/glm
  ${DEPS_DIR}/earcut/include
  ${DEPS_DIR}/isect2d/include
  ${DEPS_DIR}/hash-library
  ${DEPS_DIR}/mapbox
  ${DEPS_DIR}/pbf
  ${DEPS_DIR}/rapidjson
  ${DEPS_DIR}/sdf
  ${DEPS_DIR}/SQLiteCpp
  ${DEPS_DIR}/stb
  ${DEPS_DIR}/geojson-vt-cpp/geometry.hpp/include
  ${DEPS_DIR}/geojson-vt-cpp/geojson.hpp/include
  ${DEPS_DIR}/geojson-vt-cpp/include
  CACHE INTERNAL "core include directories" FORCE)
list(REMOVE_DUPLICATES CORE_INCLUDE_DIRS)

add_library(${CORE_LIBRARY} ${FOUND_SOURCES} ${FOUND_HEADERS})

# Build core library dependencies.
add_subdirectory(${DEPS_DIR})

# Link core library dependencies.
set(LINK_LIBRARIES
    ${CORE_LIBRARY}
    PUBLIC
    duktape
    css-color-parser-cpp
    yaml-cpp
    alfons
    SQLiteCpp
    sqlite3
    z)

if (UNIX AND NOT APPLE)
  # SQLite needs dl dynamic library loader when Linux
  target_link_libraries(${LINK_LIBRARIES} dl)
else()
  target_link_libraries(${LINK_LIBRARIES})
endif ()

target_include_directories(${CORE_LIBRARY}
  PUBLIC
  ${CORE_INCLUDE_DIRS})

target_compile_options(${CORE_LIBRARY}
  PUBLIC
  ${CORE_CXX_FLAGS})

target_compile_definitions(${CORE_LIBRARY}
  PUBLIC
  ${CORE_COMPILE_DEFS})

# make groups for xcode
group_recursive_sources(src "src")

# add compile shader targets
file(GLOB_RECURSE SHADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*")
set(SHADER_OUTPUT_DIRECTORY ${GENERATED_DIR})
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
            COMMAND ${PROJECT_SOURCE_DIR}/../scripts/incbin.sh
                ${_shader}
                core/generated/${_shader_name}.h
                ${_shader_name})
        add_dependencies(${CORE_LIBRARY} ${_shader_name})
    endif()
endforeach()

# add target to transform the default point texture into a data header
set(DEFAULT_POINT_TEXTURE_IMAGE ${CMAKE_CURRENT_SOURCE_DIR}/../images/default-point.png)
set(DEFAULT_POINT_TEXTURE_HEADER ${GENERATED_DIR}/defaultPointTextureData.h)
add_custom_target(default_point_texture DEPENDS ${DEFAULT_POINT_TEXTURE_HEADER})
add_custom_command(
  OUTPUT ${DEFAULT_POINT_TEXTURE_HEADER}
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/..
  MAIN_DEPENDENCY ${DEFAULT_POINT_TEXTURE_IMAGE}
  COMMAND ${PROJECT_SOURCE_DIR}/../scripts/incbin.sh ${DEFAULT_POINT_TEXTURE_IMAGE} ${DEFAULT_POINT_TEXTURE_HEADER} default_point_texture
  )
add_dependencies(${CORE_LIBRARY} default_point_texture)
