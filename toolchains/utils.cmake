function(check_unsupported_compiler_version)

    set(MIN_GCC 4.9)
    set(MIN_CLANG 3.4)
    set(MIN_APPLECLANG 6.0)

    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS ${MIN_GCC})
            message(FATAL_ERROR "Your GCC version does not support C++14, please install version ${MIN_GCC} or higher")
        endif()
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS ${MIN_CLANG})
            message(FATAL_ERROR "Your Clang version does not support C++14, please install version ${MIN_CLANG} or higher")
        endif()
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
        if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS ${MIN_APPLECLANG})
            message(FATAL_ERROR "Your Xcode version does not support C++14, please install version ${MIN_APPLECLANG} or higher")
        endif()
    else()
        message(WARNING "Compilation has only been tested with Clang, AppleClang, and GCC")
    endif()

endfunction(check_unsupported_compiler_version)

function(find_sources_and_include_directories HEADERS_PATH SOURCES_PATH)
    include_recursive_dirs(${HEADERS_PATH})
    file(GLOB_RECURSE FOUND_SOURCES ${SOURCES_PATH})

    set(SOURCES
        ${SOURCES}
        ${FOUND_SOURCES}
        CACHE INTERNAL "sources" FORCE)
    list(REMOVE_DUPLICATES SOURCES)
endfunction(find_sources_and_include_directories)

function(include_recursive_dirs HEADERS_PATH)
    file(GLOB_RECURSE FOUND_HEADERS ${HEADERS_PATH})

    set(INCLUDE_DIRS "")
    foreach(_headerFile ${FOUND_HEADERS})
        get_filename_component(_dir ${_headerFile} PATH)
        list(APPEND INCLUDE_DIRS ${_dir})
    endforeach()
    list(REMOVE_DUPLICATES INCLUDE_DIRS)

    include_directories(${INCLUDE_DIRS})

    set(HEADERS
        ${HEADERS}
        ${FOUND_HEADERS}
        CACHE INTERNAL "headers" FORCE)
    list(REMOVE_DUPLICATES HEADERS)
endfunction(include_recursive_dirs)

function(check_and_link_libraries TARGET)
    foreach(_lib ${ARGN})
        string(TOUPPER ${_lib} LIB)
        find_package (${_lib})
        if(${LIB}_FOUND)
            include_directories(${${LIB}_INCLUDE_DIR})
            target_link_libraries(${TARGET} ${${LIB}_LIBRARIES})
        else()
            message(SEND_ERROR "You NEED ${_lib} library.")
            return()
        endif ()
    endforeach()
endfunction(check_and_link_libraries)

macro(group_recursive_sources CURDIR CURGROUP)
    file(GLOB children ${CURDIR}/*)

    foreach(child ${children})
        if(IS_DIRECTORY ${child})
            file(GLOB FOUND_HEADERS ${child}/*.h)
            file(GLOB FOUND_SOURCES ${child}/*.cpp)
            string(REGEX MATCH "([^/]+)$" group ${child})

            if("${CURGROUP}" STREQUAL "")
                source_group(${group} FILES ${FOUND_HEADERS} ${FOUND_SOURCES})
            else()
                source_group(${CURGROUP}\\${group} FILES ${FOUND_HEADERS} ${FOUND_SOURCES})
                set(group ${CURGROUP}\\\\${group})
            endif()

            group_recursive_sources(${child} ${group})
        endif()
    endforeach()

    # add files from top level group
    file(GLOB FOUND_HEADERS ${CURGROUP}/*.h)
    file(GLOB FOUND_SOURCES ${CURGROUP}/*.cpp)
    source_group(${CURGROUP} FILES ${FOUND_HEADERS} ${FOUND_SOURCES})
endmacro()

macro(add_bundle_resources RESOURCE_LIST RESOURCE_DIR RESOURCE_BASE)

    file(GLOB_RECURSE FULL_RESOURCE_PATHS "${RESOURCE_DIR}/[^.]**")
    foreach(_full_resource_path ${FULL_RESOURCE_PATHS})
        file(RELATIVE_PATH REL_RESOURCE_PATH ${RESOURCE_DIR} ${_full_resource_path})
        get_filename_component(REL_RESOURCE_DIR ${REL_RESOURCE_PATH} PATH)
        set_source_files_properties(${_full_resource_path} PROPERTIES MACOSX_PACKAGE_LOCATION "${RESOURCE_BASE}/${REL_RESOURCE_DIR}")
        # message(STATUS "resource at: ${_full_resource_path}\n   remapped to: ${RESOURCE_BASE}/${REL_RESOURCE_DIR}")
    endforeach()
    list(APPEND ${RESOURCE_LIST} ${FULL_RESOURCE_PATHS})

endmacro(add_bundle_resources)

macro(add_resources TARGET RESOURCE_DIR)

    add_custom_command(TARGET ${TARGET}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${RESOURCE_DIR} ${CMAKE_BINARY_DIR}/bin)

endmacro(add_resources)

macro(add_framework FWNAME APPNAME LIBPATH)
    find_library(FRAMEWORK_${FWNAME} NAMES ${FWNAME} PATHS ${LIBPATH} PATH_SUFFIXES Frameworks NO_DEFAULT_PATH)
    if(${FRAMEWORK_${FWNAME}} STREQUAL FRAMEWORK_${FWNAME}-NOTFOUND)
        message(ERROR ": Framework ${FWNAME} not found")
    else()
        target_link_libraries(${APPNAME} ${FRAMEWORK_${FWNAME}})
        message(STATUS "Framework ${FWNAME} found")
    endif()
endmacro(add_framework)

