if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-gnu-zero-variadic-macro-arguments")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lc++ -lc++abi")
endif()

if (CMAKE_COMPILER_IS_GNUCC)
  message(STATUS "Using gcc ${CMAKE_CXX_COMPILER_VERSION}")
  if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 5.1)
    message(STATUS "USE CXX11_ABI")
    add_definitions("-D_GLIBCXX_USE_CXX11_ABI=1")
  endif()
  if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 7.0)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-strict-aliasing")
  endif()
endif()

check_unsupported_compiler_version()

find_package(Magnum CONFIG REQUIRED GL Sdl2Application)
find_package(CURL REQUIRED)

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

add_library(tangram SHARED
  platforms/common/platform_gl.cpp
  platforms/common/urlClient.cpp
  platforms/magnum/src/platform_magnum.cpp
)

target_include_directories(tangram
  PRIVATE
    platforms/common
    platforms/magnum/include/tangram
    ${PROJECT_BINARY_DIR}
)
target_include_directories(tangram PUBLIC 
  $<INSTALL_INTERFACE:include/tangram>
)
include(GenerateExportHeader)
generate_export_header(tangram BASE_NAME Tangram)

set_property(TARGET tangram PROPERTY CXX_STANDARD 20)

target_link_libraries(tangram
  PRIVATE
  tangram-core
  ${CURL_LIBRARIES}
  Magnum::Magnum
  Magnum::GL
  Magnum::Sdl2Application # for flextGL
)


add_resources(tangram "${PROJECT_SOURCE_DIR}/scenes" "res")

 
install(TARGETS tangram
    EXPORT TangramTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include/tangram
    PUBLIC_HEADER DESTINATION include/tangram
)

install(EXPORT TangramTargets 
    FILE TangramTargets.cmake
    DESTINATION lib/cmake/Tangram
)

include(CMakePackageConfigHelpers)
configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/TangramConfig.cmake"
    INSTALL_DESTINATION lib/cmake/Tangram
    NO_SET_AND_CHECK_MACRO
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
)
write_basic_package_version_file(
    TangramConfigVersion.cmake
    VERSION ${PACKAGE_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(EXPORT TangramTargets
    FILE TangramTargets.cmake
    NAMESPACE tangram::
    DESTINATION lib/cmake/Tangram
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/TangramConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/TangramConfigVersion.cmake
    DESTINATION lib/cmake/Tangram
)
install(DIRECTORY core/include/ TYPE INCLUDE)
install(FILES ${PROJECT_BINARY_DIR}/tangram_export.h DESTINATION include/tangram)
install(DIRECTORY platforms/magnum/include/ TYPE INCLUDE)
