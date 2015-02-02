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
