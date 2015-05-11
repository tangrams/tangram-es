# Defines the following cached variables
#   - CORE_INCLUDE_DIRS, the path where to CORE headers are located
#   - CORE_LIBRARIES_INCLUDE_DIRS, the path to CORE libraries headers

cmake_minimum_required(VERSION 2.8)
project(core)

set(INCLUDE_DIR include)
set(DEPENDENCIES_DIR dependencies)
set(SOURCE_DIR src)

add_subdirectory("${DEPENDENCIES_DIR}/tess2")

set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "Enable testing and parse tools")
add_subdirectory("${DEPENDENCIES_DIR}/yaml-cpp")

file(GLOB_RECURSE FOUND_HEADERS "${SOURCE_DIR}/*.h")
file(GLOB_RECURSE FOUND_SOURCES "${SOURCE_DIR}/*.cpp")

set(INCLUDE_DIRS "")
foreach(_headerFile ${FOUND_HEADERS})
    get_filename_component(_dir ${_headerFile} PATH)
    list(APPEND INCLUDE_DIRS ${_dir})
endforeach()
set(CORE_INCLUDE_DIRS ${INCLUDE_DIRS} CACHE INTERNAL "core include directories" FORCE)
list(APPEND INCLUDE_DIRS "${DEPENDENCIES_DIR}/tess2/libtess2/Include")
list(APPEND INCLUDE_DIRS "${DEPENDENCIES_DIR}/yaml-cpp/include")
list(APPEND INCLUDE_DIRS "${INCLUDE_DIR}/stb")
list(APPEND INCLUDE_DIRS "${INCLUDE_DIR}/glm")
list(APPEND INCLUDE_DIRS "${INCLUDE_DIR}/isect2d/include")
list(APPEND INCLUDE_DIRS "${INCLUDE_DIR}/fontstash-es/fontstash")
list(REMOVE_DUPLICATES INCLUDE_DIRS)

include_directories(${INCLUDE_DIR} ${INCLUDE_DIRS})

set(CORE_LIBRARIES_INCLUDE_DIRS
    ${PROJECT_SOURCE_DIR}/${DEPENDENCIES_DIR}/tess2/libtess2/Include/
    ${PROJECT_SOURCE_DIR}/${INCLUDE_DIR}/
    ${PROJECT_SOURCE_DIR}/${INCLUDE_DIR}/glm/
    ${PROJECT_SOURCE_DIR}/${INCLUDE_DIR}/stb/
    ${PROJECT_SOURCE_DIR}/${INCLUDE_DIR}/catch/
    ${PROJECT_SOURCE_DIR}/${INCLUDE_DIR}/fontstash-es/fontstash/
    ${PROJECT_SOURCE_DIR}/${INCLUDE_DIR}/pbf/
    ${PROJECT_SOURCE_DIR}/${INCLUDE_DIR}/rapidjson/
    CACHE INTERNAL "core libraries include directories" FORCE)

# adding some other dependent target files if needed
set(FOUND_SOURCES ${FOUND_SOURCES} ${ADDITIONNAL_TARGET_DEPENDENT_SRC_FILES})

if(NOT DEFINED CORE_LIB_NAME)
    set(CORE_LIB_NAME core)
endif()

add_library(${CORE_LIB_NAME} ${CORE_LIB_TYPE} ${FOUND_SOURCES} ${FOUND_HEADERS})
target_link_libraries(${CORE_LIB_NAME} libtess2 yaml-cpp ${CORE_LIB_DEPS})

# make groups for xcode
group_recursive_sources(src "src")

# post build commands
add_custom_command(TARGET ${CORE_LIB_NAME}
        POST_BUILD
        COMMAND ${CORE_POSTBUILD_COMMANDS}
        COMMENT ${CORE_COMMAND_MSG} VERBATIM)

# installation step
if(INSTALL_CORE_LIBRARY STREQUAL "ON")
    install(TARGETS ${CORE_LIB_NAME} DESTINATION ${CORE_INSTALLATION_PATH})
endif()
