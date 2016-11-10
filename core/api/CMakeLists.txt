
add_library(tangram-core SHARED
  ${TANGRAM_GLPROXY_HEADER_FILES}
  ${TANGRAM_GLPROXY_SOURCE_FILES}
  "${CMAKE_CURRENT_SOURCE_DIR}/tangram-core.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/urlGet_${PLATFORM_TARGET}.cpp"
  ${CMAKE_CURRENT_SOURCE_DIR}/../../external/glproxy/src/dispatch_generated.c
  ${CMAKE_CURRENT_SOURCE_DIR}/../../external/glproxy/src/dispatch_common.c
  ${PROJECT_SOURCE_DIR}/common/platform_gl.cpp
)

if (PLATFORM_TARGET STREQUAL "linux")
  list(APPEND CORE_API_CXX_FLAGS -Wall)
  list(APPEND CORE_API_CXX_FLAGS -std=c++1y)
  list(APPEND CORE_API_CXX_FLAGS -fno-omit-frame-pointer)
  list(APPEND CORE_API_CXX_FLAGS -fPIC)

  execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion
    OUTPUT_VARIABLE GCC_VERSION)
  message(STATUS "Using gcc ${GCC_VERSION}")
  if (GCC_VERSION VERSION_GREATER 5.1)
    message(STATUS "USE CXX11_ABI")
    list(APPEND CORE_API_DEFINITIONS -D_GLIBCXX_USE_CXX11_ABI=1)
  endif()
endif()

list(APPEND CORE_API_DEFINITIONS -DTANGRAM_CORE_EXPORT_NEEDED)
list(APPEND CORE_API_DEFINITIONS -DTANGRAM_USE_GLPROXY)
list(APPEND CORE_API_DEFINITIONS -DGLPROXY_IMPORTEXPORT=)
list(APPEND CORE_API_DEFINITIONS -DLOG_LEVEL=2)

target_compile_options(tangram-core
    PRIVATE
    ${CORE_API_CXX_FLAGS}
)
target_compile_definitions(tangram-core PRIVATE ${CORE_API_DEFINITIONS})
target_include_directories(tangram-core
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../../external/glproxy/include
)
if (PLATFORM_TARGET STREQUAL "linux")
  #https://bugs.launchpad.net/ubuntu/+source/gcc-5/+bug/1568899
  list(APPEND TANGRAM_CORE_LIBS -lgcc_s)
  list(APPEND TANGRAM_CORE_LIBS -lgcc)

  list(APPEND TANGRAM_CORE_LIBS -lcurl)
endif()
target_link_libraries(tangram-core
  ${CORE_LIBRARY}
  ${TANGRAM_CORE_LIBS}
)

if (CMAKE_COMPILER_IS_GNUCC)
  set(CORE_USED_LIBRARIES core duktape css-color-parser-cpp geojson-vt-cpp
    yaml-cpp alfons linebreak harfbuzz freetype icucommon
    )
  foreach(CORE_USED_LIBRARY ${CORE_USED_LIBRARIES})
    target_compile_options(${CORE_USED_LIBRARY}
      PRIVATE
      -fPIC
    )
  endforeach(CORE_USED_LIBRARY)
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
    set_target_properties(tangram-core PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/bin)
  endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )
endif()
