# - Find curl
# Find the native CURL headers and libraries.
#
#  CURL_INCLUDE_DIRS - where to find curl/curl.h, etc.
#  CURL_LIBRARIES    - List of libraries when using curl.
#  CURL_FOUND        - True if curl found.

# Look for the header file.
find_path(CURL_INCLUDE_DIR curl/curl.h
  $ENV{INCLUDE}
  "$ENV{LIB_DIR}/include"
  /usr/local/include
  /usr/include
  NO_DEFAULT_PATH)

# Look for the library.
find_library(CURL_LIBRARY NAMES curl libcurl_imp PATHS
  $ENV{LIB}
  "$ENV{LIB_DIR}/lib"
  /usr/local/lib
  /usr/lib
  NO_DEFAULT_PATH)

if(CURL_INCLUDE_DIR)
  message(STATUS "Curl include was found ${CURL_INCLUDE_DIR}")
endif(CURL_INCLUDE_DIR)
if(CURL_LIBRARY)
  message(STATUS "Curl lib was found ${CURL_LIBRARY}")
endif(CURL_LIBRARY)

# Copy the results to the output variables.
if(CURL_INCLUDE_DIR AND CURL_LIBRARY)
  set(CURL_FOUND 1)
  set(CURL_LIBRARIES ${CURL_LIBRARY})
  set(CURL_INCLUDE_DIRS ${CURL_INCLUDE_DIR})
else(CURL_INCLUDE_DIR AND CURL_LIBRARY)
  set(CURL_FOUND 0)
  set(CURL_LIBRARIES)
  set(CURL_INCLUDE_DIRS)
endif(CURL_INCLUDE_DIR AND CURL_LIBRARY)

# Report the results.
if(CURL_FOUND)
   if(NOT CURL_FIND_QUIETLY)
      message(STATUS "Found CURL: ${CURL_LIBRARY}")
   endif(NOT CURL_FIND_QUIETLY)
else(CURL_FOUND)
  set(CURL_DIR_MESSAGE "CURL was not found.")

  if(CURL_FIND_REQUIRED)
    message(FATAL_ERROR "${CURL_DIR_MESSAGE}")
  else(CURL_FIND_REQUIRED)
    if(NOT CURL_FIND_QUIETLY)
      message(STATUS "${CURL_DIR_MESSAGE}")
    endif(NOT CURL_FIND_QUIETLY)
    set(CURL_INCLUDE_DIR "")
    set(CURL_LIBRARY "")
  endif(CURL_FIND_REQUIRED)
endif(CURL_FOUND)