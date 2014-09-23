include(${CMAKE_SOURCE_DIR}/build/toolchains/iOS.toolchain.cmake)

macro(add_framework FWNAME APPNAME LIBPATH)
    find_library(FRAMEWORK_${FWNAME} NAMES ${FWNAME} PATHS ${LIBPATH} PATH_SUFFIXES Frameworks NO_DEFAULT_PATH)
    if(${FRAMEWORK_${FWNAME}} STREQUAL FRAMEWORK_${FWNAME}-NOTFOUND)
        message(ERROR ": Framework ${FWNAME} not found")
    else()
        target_link_libraries(${APPNAME} ${FRAMEWORK_${FWNAME}})
        message(STATUS "Framework ${FWNAME} found at ${FRAMEWORK_${FWNAME}}")
    endif()
endmacro(add_framework)

add_definitions(-DPLATFORM_IOS=1)

set(EXECUTABLE_NAME "tangram")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fobjc-abi-version=2 -fobjc-arc -std=gnu++11 -stdlib=libc++ -isysroot ${CMAKE_OSX_SYSROOT}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fobjc-abi-version=2 -fobjc-arc -isysroot ${CMAKE_OSX_SYSROOT}")

if(${SIMULATOR})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mios-simulator-version-min=6.0")
endif()

set(SDKVER "8.0")
set(LIBPATH "/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk/System/Library/Frameworks")
set(DEVROOT "/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer")
set(SDKROOT "${DEVROOT}/SDKs/iPhoneOS${SDKVER}.sdk")

set(CMAKE_EXE_LINKER_FLAGS 
    "-framework CoreGraphics 
     -framework CoreFoundation
     -framework QuartzCore 
     -framework UIKit 
     -framework OpenGLES")

set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.mapzen.\${PRODUCT_NAME:Tangram}")
set(APP_TYPE MACOSX_BUNDLE)

# load core sources
set(INCLUDE_CORE_DIR ${PROJECT_SOURCE_DIR}/core/include)

add_subdirectory("${INCLUDE_CORE_DIR}/json")
add_subdirectory("${INCLUDE_CORE_DIR}/glm")

include_directories(${INCLUDE_CORE_DIR}/json)
include_directories(${PROJECT_SOURCE_DIR}/core/include/)
find_sources_and_include_directories(
    ${PROJECT_SOURCE_DIR}/core/src/*.h
    ${PROJECT_SOURCE_DIR}/core/src/*.cpp)

# ios source files
set(IOS_EXTENSIONS_FILES *.mm *.cpp *.m)
foreach(_ext ${IOS_EXTENSIONS_FILES})
    find_sources_and_include_directories(
        ${PROJECT_SOURCE_DIR}/ios/*.h 
        ${PROJECT_SOURCE_DIR}/ios/${_ext})
endforeach()

# link and build functions
function(link_libraries)
    #check_and_link_libraries(${EXECUTABLE_NAME} curl)
    target_link_libraries(${EXECUTABLE_NAME} core)
    
    add_framework(Foundation ${EXECUTABLE_NAME} ${LIBPATH})
    add_framework(CoreGraphics ${EXECUTABLE_NAME} ${LIBPATH})
    add_framework(CoreFoundation ${EXECUTABLE_NAME} ${LIBPATH})
    add_framework(UIKit ${EXECUTABLE_NAME} ${LIBPATH})
    add_framework(OpenGLES ${EXECUTABLE_NAME} ${LIBPATH})
endfunction()

function(build)
    add_executable(${EXECUTABLE_NAME} ${APP_TYPE} ${HEADERS} ${SOURCES})

    set_xcode_property(json GCC_GENERATE_DEBUGGING_SYMBOLS YES)
    set_xcode_property(${EXECUTABLE_NAME} GCC_GENERATE_DEBUGGING_SYMBOLS YES)
endfunction()
