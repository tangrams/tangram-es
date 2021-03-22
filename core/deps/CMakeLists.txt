if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  # Recent versions of Clang are extremely verbose about implicit long-to-int
  # conversions. While this *can* be an actual error, it's not practical to
  # address every instance of it in all dependencies, so we'll silence it here.
  add_compile_options(-Wno-shorten-64-to-32)
endif()

## glm ##
#########
find_package(glm CONFIG REQUIRED)

## yaml-cpp ##
##############
find_package(yaml-cpp CONFIG REQUIRED)

## css-color-parser-cpp ##
##########################
add_library(css-color-parser-cpp css-color-parser-cpp/csscolorparser.cpp)

set_target_properties(css-color-parser-cpp PROPERTIES CXX_STANDARD 14 CXX_STANDARD_REQUIRED ON)

target_include_directories(css-color-parser-cpp PUBLIC css-color-parser-cpp)

## duktape ##
#############
find_package(duktape CONFIG REQUIRED)


## miniz ##
###########
find_package(miniz CONFIG REQUIRED)



if(NOT TANGRAM_USE_SYSTEM_FONT_LIBS)
  ## Harfbuzz - ICU-Common - Freetype2 ##
  #######################################
  find_package(harfbuzz CONFIG REQUIRED)

  message(STATUS "harfbuzz" ${HARFBUZZ_LIBRARIES})

  set(ALFONS_DEPS_LIBRARIES
    ${ALFONS_DEPS_LIBRARIES}
    harfbuzz ${HARFBUZZ_LIBRARIES}
    CACHE INTERNAL "alfons-libs" FORCE)
endif()

## alfons ##
############
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/alfons)
target_compile_definitions(alfons PRIVATE GLM_FORCE_CTOR_INIT)

if (TANGRAM_MBTILES_DATASOURCE)
  ## SQLiteCpp ##
  ###############
  find_package(SQLiteCpp CONFIG REQUIRED)
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
