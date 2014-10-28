cmake_minimum_required(VERSION 2.8)
project(core)

set(INCLUDE_DIR include)
set(SOURCE_DIR src)

add_subdirectory("${INCLUDE_DIR}/jsoncpp")
add_subdirectory("${INCLUDE_DIR}/glm")

file(GLOB_RECURSE FOUND_HEADERS "${SOURCE_DIR}/*.h")
file(GLOB_RECURSE FOUND_SOURCES "${SOURCE_DIR}/*.cpp")

set(INCLUDE_DIRS "")
foreach(_headerFile ${FOUND_HEADERS})
    get_filename_component(_dir ${_headerFile} PATH)
    list(APPEND INCLUDE_DIRS ${_dir})
endforeach()
list(REMOVE_DUPLICATES INCLUDE_DIRS)

include_directories(${INCLUDE_DIR}/jsoncpp ${INCLUDE_DIR} ${INCLUDE_DIRS})

# adding some other dependent target files if needed
set(FOUND_SOURCES ${FOUND_SOURCES} ${ADDITIONNAL_TARGET_DEPENDENT_SRC_FILES})

if(NOT DEFINED CORE_LIB_NAME)
    set(CORE_LIB_NAME core)
endif()

add_library(${CORE_LIB_NAME} ${CORE_LIB_TYPE} ${FOUND_SOURCES})
target_link_libraries(${CORE_LIB_NAME} json ${CORE_LIB_DEPS})

# post build commands
add_custom_command(TARGET ${CORE_LIB_NAME}
        POST_BUILD
        COMMAND ${CORE_POSTBUILD_COMMANDS}
        COMMENT ${CORE_COMMAND_MSG} VERBATIM)

# installation step
if(INSTALL_CORE_LIBRARY STREQUAL "ON")
    install(TARGETS ${CORE_LIB_NAME} DESTINATION ${CORE_INSTALLATION_PATH})
endif()
