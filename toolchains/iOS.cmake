include(${CMAKE_SOURCE_DIR}/toolchains/iOS.toolchain.cmake)

add_definitions(-DPLATFORM_IOS)

set(EXECUTABLE_NAME "tangram")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}
    -fobjc-abi-version=2
    -fobjc-arc
    -std=c++1y
    -stdlib=libc++
    -isysroot ${CMAKE_IOS_SDK_ROOT}")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}
    -fobjc-abi-version=2
    -fobjc-arc
    -isysroot ${CMAKE_IOS_SDK_ROOT}")

if(${IOS_PLATFORM} STREQUAL "SIMULATOR")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mios-simulator-version-min=6.0")
    set(ARCH "i386")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    set(ARCH "armv7 armv7s arm64")
endif()

set(FRAMEWORKS CoreGraphics CoreFoundation QuartzCore UIKit OpenGLES Security CFNetwork GLKit)
set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.mapzen.\${PRODUCT_NAME:Tangram}")
set(APP_TYPE MACOSX_BUNDLE)

# load core library
add_subdirectory(${PROJECT_SOURCE_DIR}/external)
add_subdirectory(${PROJECT_SOURCE_DIR}/core)

# ios source files
set(IOS_EXTENSIONS_FILES *.mm *.cpp *.m)
foreach(_ext ${IOS_EXTENSIONS_FILES})
    find_sources_and_include_directories(
        ${PROJECT_SOURCE_DIR}/ios/src/*.h
        ${PROJECT_SOURCE_DIR}/ios/src/${_ext})
    find_sources_and_include_directories(
        ${PROJECT_SOURCE_DIR}/ios/demo/src/*.h
        ${PROJECT_SOURCE_DIR}/ios/demo/src/${_ext})
endforeach()

add_bundle_resources(RESOURCES "${PROJECT_SOURCE_DIR}/scenes" "${EXECUTABLE_NAME}.app")

file(GLOB_RECURSE IOS_DEMO_RESOURCES "${PROJECT_SOURCE_DIR}/ios/demo/resources/[^.]**")

macro(add_framework FWNAME APPNAME LIBPATH)
    find_library(FRAMEWORK_${FWNAME} NAMES ${FWNAME} PATHS ${LIBPATH} PATH_SUFFIXES Frameworks NO_DEFAULT_PATH)
    if(${FRAMEWORK_${FWNAME}} STREQUAL FRAMEWORK_${FWNAME}-NOTFOUND)
        message(ERROR ": Framework ${FWNAME} not found")
    else()
        target_link_libraries(${APPNAME} ${FRAMEWORK_${FWNAME}})
        message(STATUS "Framework ${FWNAME} found")
    endif()
endmacro(add_framework)

set(SOURCES ${SOURCES} ${PROJECT_SOURCE_DIR}/core/common/platform_gl.cpp)

file(GLOB_RECURSE IOS_DEMO_SOURCES ${PROJECT_SOURCE_DIR}/ios/demo/src/**)
file(GLOB_RECURSE TANGRAM_SOURCES ${PROJECT_SOURCE_DIR}/ios/src/**)
source_group("Resources\\tangram" FILES ${RESOURCES})
source_group("Resources\\demo" FILES ${IOS_DEMO_RESOURCES})
source_group("src" FILES ${IOS_DEMO_SOURCES})
source_group("platform" FILES ${TANGRAM_SOURCES} ${PROJECT_SOURCE_DIR}/core/common/platform_gl.cpp)

add_executable(${EXECUTABLE_NAME} ${APP_TYPE} ${HEADERS} ${SOURCES} ${RESOURCES} ${IOS_DEMO_RESOURCES})

target_link_libraries(${EXECUTABLE_NAME}
    ${CORE_LIBRARY})

# setting xcode properties
set_target_properties(${EXECUTABLE_NAME} PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${PROJECT_SOURCE_DIR}/ios/demo/resources/tangram-Info.plist
    RESOURCE "${IOS_RESOURCES}")

set_xcode_property(${EXECUTABLE_NAME} GCC_GENERATE_DEBUGGING_SYMBOLS YES)
set_xcode_property(${EXECUTABLE_NAME} SUPPORTED_PLATFORMS "iphonesimulator iphoneos")
set_xcode_property(${EXECUTABLE_NAME} ONLY_ACTIVE_ARCH "YES")
set_xcode_property(${EXECUTABLE_NAME} VALID_ARCHS "${ARCH}")
set_xcode_property(${EXECUTABLE_NAME} TARGETED_DEVICE_FAMILY "1,2")

foreach(_framework ${FRAMEWORKS})
    add_framework(${_framework} ${EXECUTABLE_NAME} ${CMAKE_SYSTEM_FRAMEWORK_PATH})
endforeach()
