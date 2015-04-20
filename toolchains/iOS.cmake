include(${CMAKE_SOURCE_DIR}/toolchains/iOS.toolchain.cmake)

add_definitions(-DPLATFORM_IOS)

set(EXECUTABLE_NAME "tangram")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} 
    -fobjc-abi-version=2 
    -fobjc-arc 
    -std=gnu++11 
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
    set(ARCH "armv6 armv7")
endif()

set(FRAMEWORKS CoreGraphics CoreFoundation QuartzCore UIKit OpenGLES Security CFNetwork GLKit) 
set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.mapzen.\${PRODUCT_NAME:Tangram}")
set(APP_TYPE MACOSX_BUNDLE)

file(GLOB_RECURSE RESOURCES ${PROJECT_SOURCE_DIR}/ios/resources/**)
file(GLOB_RECURSE CORE_RESOURCES ${PROJECT_SOURCE_DIR}/core/resources/**)
list(APPEND RESOURCES ${CORE_RESOURCES})
string(REGEX REPLACE "[.]DS_Store" "" RESOURCES "${RESOURCES}")

# load core library
add_subdirectory(${PROJECT_SOURCE_DIR}/core)
include_directories(${CORE_INCLUDE_DIRS})

find_package(ZLIB REQUIRED)
if (ZLIB_FOUND)
    include_directories(${ZLIB_INCLUDE_DIRS})
endif()

# ios source files
set(IOS_EXTENSIONS_FILES *.mm *.cpp *.m)
foreach(_ext ${IOS_EXTENSIONS_FILES})
    find_sources_and_include_directories(
        ${PROJECT_SOURCE_DIR}/ios/src/*.h 
        ${PROJECT_SOURCE_DIR}/ios/src/${_ext})
endforeach()

# link and build functions
function(link_libraries)
    target_link_libraries(${EXECUTABLE_NAME} core)
    target_link_libraries(${EXECUTABLE_NAME} ${ZLIB_LIBRARIES})
    
    foreach(_framework ${FRAMEWORKS})
        add_framework(${_framework} ${EXECUTABLE_NAME} ${CMAKE_SYSTEM_FRAMEWORK_PATH})
    endforeach()
endfunction()

function(build)
    add_executable(${EXECUTABLE_NAME} ${APP_TYPE} ${HEADERS} ${SOURCES} ${RESOURCES})

    # setting xcode properties
    set_target_properties(${EXECUTABLE_NAME} PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST ${PROJECT_SOURCE_DIR}/ios/resources/tangram-Info.plist
        RESOURCE "${RESOURCES}")

    set_xcode_property(${EXECUTABLE_NAME} GCC_GENERATE_DEBUGGING_SYMBOLS YES)
    set_xcode_property(${EXECUTABLE_NAME} SUPPORTED_PLATFORMS "iphonesimulator iphoneos")
    set_xcode_property(${EXECUTABLE_NAME} ONLY_ACTIVE_ARCH "NO")
    set_xcode_property(${EXECUTABLE_NAME} VALID_ARCHS "${ARCH}")
    set_xcode_property(${EXECUTABLE_NAME} TARGETED_DEVICE_FAMILY "1,2")
endfunction()

macro(add_framework FWNAME APPNAME LIBPATH)
    find_library(FRAMEWORK_${FWNAME} NAMES ${FWNAME} PATHS ${LIBPATH} PATH_SUFFIXES Frameworks NO_DEFAULT_PATH)
    if(${FRAMEWORK_${FWNAME}} STREQUAL FRAMEWORK_${FWNAME}-NOTFOUND)
        message(ERROR ": Framework ${FWNAME} not found")
    else()
        target_link_libraries(${APPNAME} ${FRAMEWORK_${FWNAME}})
        message(STATUS "Framework ${FWNAME} found")
    endif()
endmacro(add_framework)
