include(${CMAKE_SOURCE_DIR}/toolchains/iOS.toolchain.cmake)

message(STATUS "Build for iOS archs " ${CMAKE_OSX_ARCHITECTURES})

set(FRAMEWORK_NAME TangramMap)
set(FRAMEWORK_VERSION "1.0")

add_definitions(-DPLATFORM_IOS)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}
    -fobjc-abi-version=2
    -fobjc-arc
    -std=c++14
    -stdlib=libc++
    -w
    -isysroot ${CMAKE_IOS_SDK_ROOT}")

set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}
    -fobjc-abi-version=2
    -fobjc-arc
    -w
    -isysroot ${CMAKE_IOS_SDK_ROOT}")

if(${IOS_PLATFORM} STREQUAL "SIMULATOR")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mios-simulator-version-min=6.0")
    set(ARCH "i386 x86_64")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    set(ARCH "armv7 armv7s arm64")
endif()

set(FRAMEWORKS CoreGraphics CoreFoundation QuartzCore UIKit OpenGLES Security CFNetwork GLKit)

# load core library
add_subdirectory(${PROJECT_SOURCE_DIR}/external)
add_subdirectory(${PROJECT_SOURCE_DIR}/core)

set(SOURCES
    ${PROJECT_SOURCE_DIR}/core/common/platform_gl.cpp
    ${PROJECT_SOURCE_DIR}/ios/src/platform_ios.mm
    ${PROJECT_SOURCE_DIR}/ios/src/TGMapViewController.mm)

set(HEADERS
    ${PROJECT_SOURCE_DIR}/ios/src/platform_ios.h
    ${PROJECT_SOURCE_DIR}/ios/framework/TangramMap.h
    ${PROJECT_SOURCE_DIR}/ios/src/TGMapViewDelegate.h
    ${PROJECT_SOURCE_DIR}/ios/src/TGMapViewController.h)

set(FRAMEWORK_HEADERS
    ${PROJECT_SOURCE_DIR}/ios/framework/TangramMap.h
    ${PROJECT_SOURCE_DIR}/ios/src/TGMapViewDelegate.h
    ${PROJECT_SOURCE_DIR}/ios/src/TGMapViewController.h)

add_bundle_resources(RESOURCES "${PROJECT_SOURCE_DIR}/scenes/fonts" "./fonts")
add_bundle_resources(RESOURCES "${PROJECT_SOURCE_DIR}/ios/framework/Modules" "./Modules")

add_library(${FRAMEWORK_NAME} SHARED ${SOURCES} ${HEADERS} ${RESOURCES})
target_link_libraries(${FRAMEWORK_NAME} ${CORE_LIBRARY})

set(IOS_FRAMEWORK_RESOURCES ${PROJECT_SOURCE_DIR}/ios/framework/Info.plist)

set_target_properties(${FRAMEWORK_NAME} PROPERTIES
    CLEAN_DIRECT_OUTPUT 1
    FRAMEWORK TRUE
    FRAMEWORK_VERSION ${FRAMEWORK_VERSION}
    MACOSX_FRAMEWORK_IDENTIFIER com.mapzen.tangramMap
    MACOSX_FRAMEWORK_INFO_PLIST ${IOS_FRAMEWORK_RESOURCES}
    #VERSION 1.0.0
    #SOVERSION 1.0.0
    PUBLIC_HEADER "${FRAMEWORK_HEADERS}"
    RESOURCE "${IOS_FRAMEWORK_RESOURCES}"
    )

set_xcode_property(${FRAMEWORK_NAME} CODE_SIGN_IDENTITY "")
set_xcode_property(${FRAMEWORK_NAME} CODE_SIGNING_REQUIRED "NO")
set_xcode_property(${FRAMEWORK_NAME} CODE_SIGN_ENTITLEMENTS "")
set_xcode_property(${FRAMEWORK_NAME} CODE_SIGNING_ALLOWED "NO")

set_xcode_property(${FRAMEWORK_NAME} ENABLE_BITCODE "YES")
set_xcode_property(${FRAMEWORK_NAME} SUPPORTED_PLATFORMS "iphonesimulator iphoneos")
set_xcode_property(${FRAMEWORK_NAME} ONLY_ACTIVE_ARCH "NO")
set_xcode_property(${FRAMEWORK_NAME} VALID_ARCHS "${ARCH}")
set_xcode_property(${FRAMEWORK_NAME} ARCHS "${ARCH}")

# Set RPATH to be within the application /Frameworks directory
set_xcode_property(${FRAMEWORK_NAME} LD_DYLIB_INSTALL_NAME "@rpath/${FRAMEWORK_NAME}.framework/${FRAMEWORK_NAME}")

foreach(_framework ${FRAMEWORKS})
    add_framework(${_framework} ${FRAMEWORK_NAME} ${CMAKE_SYSTEM_FRAMEWORK_PATH})
endforeach()

