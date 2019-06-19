if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  # Recent versions of Clang are extremely verbose about implicit long-to-int
  # conversions. While this *can* be an actual error, it's not practical to
  # address every instance of it in all dependencies, so we'll silence it here.
  add_compile_options(-Wno-shorten-64-to-32)
endif()

## yaml-cpp ##
##############
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "")
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "")
set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "")
set(YAML_CPP_INSTALL OFF CACHE BOOL "")
add_subdirectory(yaml-cpp)

target_include_directories(yaml-cpp PUBLIC yaml-cpp/include)

## css-color-parser-cpp ##
##########################
add_library(css-color-parser-cpp css-color-parser-cpp/csscolorparser.cpp)

set_target_properties(css-color-parser-cpp PROPERTIES CXX_STANDARD 14 CXX_STANDARD_REQUIRED ON)

target_include_directories(css-color-parser-cpp PUBLIC css-color-parser-cpp)

## duktape ##
#############
add_subdirectory(duktape)


## miniz ##
###########
add_library(miniz
    ${CMAKE_CURRENT_SOURCE_DIR}/miniz/miniz.c)

target_include_directories(miniz
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/miniz)



if(NOT TANGRAM_USE_SYSTEM_FONT_LIBS)
  ## Harfbuzz - ICU-Common - UCDN - Freetype2 ##
  ##############################################
  set(HARFBUZZ_BUILD_ICU ON CACHE BOOL "Enable building of ICU")
  set(HARFBUZZ_BUILD_UCDN ON CACHE BOOL "Enable building of UCDN")
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/harfbuzz-icu-freetype)

  message(STATUS "harfbuzz" ${HARFBUZZ_LIBRARIES})

  set(ALFONS_DEPS_LIBRARIES
    ${ALFONS_DEPS_LIBRARIES}
    harfbuzz ${HARFBUZZ_LIBRARIES}
    CACHE INTERNAL "alfons-libs" FORCE)
endif()

## alfons ##
############
set(GLM_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/glm)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/alfons)

if (TANGRAM_MBTILES_DATASOURCE)
  ## SQLiteCpp ##
  ###############
  set(SQLITECPP_RUN_CPPLINT OFF CACHE BOOL "")
  set(SQLITECPP_RUN_CPPCHECK OFF CACHE BOOL "")
  set(SQLITE_ENABLE_COLUMN_METADATA OFF CACHE BOOL "")

  if (TANGRAM_USE_SYSTEM_SQLITE_LIBS)
    set(SQLITECPP_INTERNAL_SQLITE OFF CACHE BOOL "")
  endif()

  add_subdirectory(SQLiteCpp)

  # Extensions aren't needed for MBTiles and aren't available in older versions of sqlite3.
  target_compile_definitions(SQLiteCpp PRIVATE SQLITE_OMIT_LOAD_EXTENSION)

  # needed for sqlite3 to work for ndk15c+ and android api level < 21
  # refer:
  # https://github.com/android-ndk/ndk/issues/477 and
  # https://android.googlesource.com/platform/bionic/+/master/docs/32-bit-abi.md
  if (ANDROID)
    target_compile_definitions(sqlite3 PRIVATE _FILE_OFFSET_BITS=32)
  endif()
  if (WIN32)
    target_compile_options(SQLiteCpp PRIVATE -fno-stack-protector)
    target_compile_options(sqlite3 PRIVATE -fno-stack-protector)
  endif()
endif()

## double-conversion ##
#######################
add_subdirectory(double-conversion)


## Prebuild JavascriptCore ##
#############################
if (TANGRAM_USE_JSCORE_STATIC)
  add_library(jscore-static STATIC IMPORTED GLOBAL)
  if(NOT EXISTS "${PROJECT_SOURCE_DIR}/deps/jscore/lib/libJavaScriptCore.a")
    message(SEND_ERROR "Please add static JavaScriptCore libary to core/deps/jscore/")
  endif()
  find_package(ICU COMPONENTS io i18n REQUIRED)
  link_directories(${ICU_LIBRARY_DIRS})

  set(JSCORE_LIBS
    ${ICU_LIBRARIES}
    "${CMAKE_CURRENT_SOURCE_DIR}/jscore/lib/libJavaScriptCore.a"
    "${CMAKE_CURRENT_SOURCE_DIR}/jscore/lib/libWTF.a"
    "${CMAKE_CURRENT_SOURCE_DIR}/jscore/lib/libbmalloc.a")

  set_target_properties(jscore-static
         PROPERTIES
         IMPORTED_LOCATION
         ${JSCORE_LIBS})

  target_link_libraries(jscore-static
    INTERFACE
    ${JSCORE_LIBS})

  target_include_directories(jscore-static
    INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/jscore/include")
endif()
