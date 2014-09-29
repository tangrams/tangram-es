include(${CMAKE_SOURCE_DIR}/build/toolchains/iOS.toolchain.cmake)

macro(add_framework FWNAME APPNAME LIBPATH)
    find_library(FRAMEWORK_${FWNAME} NAMES ${FWNAME} PATHS ${LIBPATH} PATH_SUFFIXES Frameworks NO_DEFAULT_PATH)
    if(${FRAMEWORK_${FWNAME}} STREQUAL FRAMEWORK_${FWNAME}-NOTFOUND)
        message(ERROR ": Framework ${FWNAME} not found")
    else()
        target_link_libraries(${APPNAME} ${FRAMEWORK_${FWNAME}})
        message(STATUS "Framework ${FWNAME} found")
    endif()
endmacro(add_framework)

add_definitions(-DPLATFORM_IOS)

set(EXECUTABLE_NAME "tangram")

# uncomment to remove ZERO_CHECK from xcode
# set(CMAKE_SUPPRESS_REGENERATION TRUE)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} 
    -fobjc-abi-version=2 
    -fobjc-arc 
    -std=gnu++11 
    -stdlib=libc++ 
    -isysroot ${CMAKE_IOS_SDK_ROOT}")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} 
    -fobjc-abi-version=2 
    -fobjc-arc 
    -isysroot ${CMAKE_IOS_SDK_ROOT}")

if(${IOS_PLATFORM} STREQUAL "SIMULATOR")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mios-simulator-version-min=6.0")
    set(ARCH "i386")
endif()

set(FRAMEWORKS CoreGraphics CoreFoundation QuartzCore UIKit OpenGLES Security CFNetwork GLKit) 
set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.mapzen.\${PRODUCT_NAME:Tangram}")
set(APP_TYPE MACOSX_BUNDLE)
set(CORE_LIB_DEPS ${CMAKE_SOURCE_DIR}/ios/precompiled/libcurl.a)

# include headers for ios
if(${IOS_PLATFORM} STREQUAL "SIMULATOR")
    include_directories(${PROJECT_SOURCE_DIR}/ios/include/)
endif()

# load core library
include_directories(${PROJECT_SOURCE_DIR}/core/include/)
add_subdirectory(${PROJECT_SOURCE_DIR}/core)
include_recursive_dirs(${PROJECT_SOURCE_DIR}/core/*.h)

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
    add_executable(${EXECUTABLE_NAME} ${APP_TYPE} ${HEADERS} ${SOURCES})

    set_xcode_property(json GCC_GENERATE_DEBUGGING_SYMBOLS YES)
    set_xcode_property(${EXECUTABLE_NAME} GCC_GENERATE_DEBUGGING_SYMBOLS YES)
    set_xcode_property(${EXECUTABLE_NAME} SUPPORTED_PLATFORMS "iphonesimulator iphoneos")
    set_xcode_property(${EXECUTABLE_NAME} ONLY_ACTIVE_ARCH "NO")
    set_xcode_property(${EXECUTABLE_NAME} VALID_ARCHS "${ARCH}")
endfunction()
