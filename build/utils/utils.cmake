function(find_sources_and_include_directories HEADERS_PATH SOURCES_PATH)
    file(GLOB_RECURSE FOUND_SOURCES ${SOURCES_PATH})
    file(GLOB_RECURSE FOUND_HEADERS ${HEADERS_PATH})

    set(INCLUDE_DIRS "")
    foreach(_headerFile ${FOUND_HEADERS})
        get_filename_component(_dir ${_headerFile} PATH)
        list(APPEND INCLUDE_DIRS ${_dir})
    endforeach()
    list(REMOVE_DUPLICATES INCLUDE_DIRS)

    include_directories(${INCLUDE_DIRS})

    set(SOURCES
        ${SOURCES}
        ${FOUND_SOURCES}
        PARENT_SCOPE)
endfunction(find_sources_and_include_directories)

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
