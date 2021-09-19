
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
# Recent versions of Clang are extremely verbose about implicit long-to-int
# conversions. While this *can* be an actual error, it's not practical to
# address every instance of it in all dependencies, so we'll silence it here.
add_compile_options(-Wno-shorten-64-to-32)
endif()

## hash-library ##
##########################
add_subdirectory(hash-library)

## sdf ##
##########################
add_library(sdf INTERFACE)
target_include_directories(sdf INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/sdf)

## pbf ##
##########################
add_library(pbf INTERFACE)
target_include_directories(pbf INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

## earcut ##
##########################
add_library(earcut INTERFACE)
target_include_directories(earcut INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/earcut/include)

## isect2d ##
##########################
add_library(isect2d INTERFACE)
target_include_directories(isect2d INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/isect2d/include)

## css-color-parser-cpp ##
##########################
add_library(css-color-parser-cpp css-color-parser-cpp/csscolorparser.cpp)
set_target_properties(css-color-parser-cpp PROPERTIES CXX_STANDARD 14 CXX_STANDARD_REQUIRED ON)
target_include_directories(css-color-parser-cpp PUBLIC css-color-parser-cpp)

set(ALFONS_DEPS_LIBRARIES glm::glm)
if(NOT TANGRAM_USE_SYSTEM_FONT_LIBS)
  ## Harfbuzz - ICU-Common - Freetype2 ##
  #######################################
  find_package(harfbuzz CONFIG REQUIRED)
  list(APPEND ALFONS_DEPS_LIBRARIES harfbuzz::harfbuzz)
endif()



## alfons ##
############
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/alfons)
target_compile_definitions(alfons PRIVATE GLM_FORCE_CTOR_INIT)
if(MSVC)
  # Use a lower warning level on MSVC because -Wall produces excessive output.
  target_compile_options(alfons PUBLIC "/W1")
endif()


## Prebuild JavascriptCore ##
#############################
if (TANGRAM_USE_JSCORE_STATIC)
add_library(jscore-static STATIC IMPORTED GLOBAL)
if(NOT EXISTS "${PROJECT_SOURCE_DIR}/deps/jscore/lib/libJavaScriptCore.a")
  message(SEND_ERROR "Please add static JavaScriptCore libary to core/deps/jscore/")
endif()

set(JSCORE_LIBS
  ICU::io
  ICU::i18n
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
