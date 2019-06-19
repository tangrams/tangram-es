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

function(get_nextzen_api_key KEY_RESULT)
  set(${KEY_RESULT} $ENV{NEXTZEN_API_KEY} PARENT_SCOPE)
  if(${KEY_RESULT} STREQUAL "")
    message(SEND_ERROR
      "Make sure to provide an api key to build the demo application, "
      "you can create an API key at https://developers.nextzen.org/. "
      "Then run 'export NEXTZEN_API_KEY yourKeyHere' or specify `NEXTZEN_API_KEY=yourKeyHere` as an argument to the make command")
    return()
  endif()
endfunction(get_nextzen_api_key)

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

macro(add_resources TARGET RESOURCE_DIR DEST_DIR)
  add_custom_command(TARGET ${TARGET}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${RESOURCE_DIR} ${CMAKE_BINARY_DIR}/${DEST_DIR})
endmacro(add_resources)
