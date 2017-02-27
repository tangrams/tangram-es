include(${CMAKE_SOURCE_DIR}/toolchains/iOS.toolchain.cmake)

message(STATUS "Build for iOS archs " ${IOS_ARCH})

set(FRAMEWORK_NAME TangramMap)

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
else()
    if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fembed-bitcode")
        add_compile_options("-fembed-bitcode")
    endif()
endif()

set(FRAMEWORKS CoreGraphics CoreFoundation QuartzCore UIKit OpenGLES Security CFNetwork GLKit)

# Tell SQLiteCpp to not build its own copy of SQLite, we will use the system library instead.
set(SQLITECPP_INTERNAL_SQLITE OFF)

# load core library
add_subdirectory(${PROJECT_SOURCE_DIR}/core)

set(SOURCES
    ${PROJECT_SOURCE_DIR}/platforms/common/platform_gl.cpp
    ${PROJECT_SOURCE_DIR}/platforms/ios/src/TangramMap/platform_ios.mm
    ${PROJECT_SOURCE_DIR}/platforms/ios/src/TangramMap/TGHelpers.mm
    ${PROJECT_SOURCE_DIR}/platforms/ios/src/TangramMap/TGFontConverter.mm
    ${PROJECT_SOURCE_DIR}/platforms/ios/src/TangramMap/TGGeoPolyline.mm
    ${PROJECT_SOURCE_DIR}/platforms/ios/src/TangramMap/TGGeoPolygon.mm
    ${PROJECT_SOURCE_DIR}/platforms/ios/src/TangramMap/TGHttpHandler.mm
    ${PROJECT_SOURCE_DIR}/platforms/ios/src/TangramMap/TGMapData.mm
    ${PROJECT_SOURCE_DIR}/platforms/ios/src/TangramMap/TGSceneUpdate.mm
    ${PROJECT_SOURCE_DIR}/platforms/ios/src/TangramMap/TGLabelPickResult.mm
    ${PROJECT_SOURCE_DIR}/platforms/ios/src/TangramMap/TGMarkerPickResult.mm
    ${PROJECT_SOURCE_DIR}/platforms/ios/src/TangramMap/TGMarker.mm
    ${PROJECT_SOURCE_DIR}/platforms/ios/src/TangramMap/TGMapViewController.mm)

set(FRAMEWORK_HEADERS
    ${PROJECT_SOURCE_DIR}/platforms/ios/framework/TangramMap.h
    ${PROJECT_SOURCE_DIR}/platforms/ios/src/TangramMap/TGGeoPolyline.h
    ${PROJECT_SOURCE_DIR}/platforms/ios/src/TangramMap/TGGeoPolygon.h
    ${PROJECT_SOURCE_DIR}/platforms/ios/src/TangramMap/TGGeoPoint.h
    ${PROJECT_SOURCE_DIR}/platforms/ios/src/TangramMap/TGMarker.h
    ${PROJECT_SOURCE_DIR}/platforms/ios/src/TangramMap/TGEaseType.h
    ${PROJECT_SOURCE_DIR}/platforms/ios/src/TangramMap/TGSceneUpdate.h
    ${PROJECT_SOURCE_DIR}/platforms/ios/src/TangramMap/TGMapData.h
    ${PROJECT_SOURCE_DIR}/platforms/ios/src/TangramMap/TGHttpHandler.h
    ${PROJECT_SOURCE_DIR}/platforms/ios/src/TangramMap/TGLabelPickResult.h
    ${PROJECT_SOURCE_DIR}/platforms/ios/src/TangramMap/TGMarkerPickResult.h
    ${PROJECT_SOURCE_DIR}/platforms/ios/src/TangramMap/TGMapViewController.h)

set(HEADERS
    ${PROJECT_SOURCE_DIR}/platforms/ios/src/TangramMap/platform_ios.h
    ${PROJECT_SOURCE_DIR}/platforms/ios/src/TangramMap/TGMarkerPickResult+Internal.h
    ${PROJECT_SOURCE_DIR}/platforms/ios/src/TangramMap/TGLabelPickResult+Internal.h
    ${PROJECT_SOURCE_DIR}/platforms/ios/src/TangramMap/TGMapViewController+Internal.h
    ${PROJECT_SOURCE_DIR}/platforms/ios/src/TangramMap/TGMapData+Internal.h
    ${PROJECT_SOURCE_DIR}/platforms/ios/src/TangramMap/TGHelpers.h
    ${PROJECT_SOURCE_DIR}/platforms/ios/src/TangramMap/TGFontConverter.h
    ${FRAMEWORK_HEADERS})

# add_bundle_resources(RESOURCES "${PROJECT_SOURCE_DIR}/scenes/fonts" "./fonts")
add_bundle_resources(RESOURCES "${PROJECT_SOURCE_DIR}/platforms/ios/framework/Modules" "./Modules")

add_library(${FRAMEWORK_NAME} SHARED ${SOURCES} ${HEADERS} ${RESOURCES})
target_link_libraries(${FRAMEWORK_NAME} ${CORE_LIBRARY})

# Link with SQLite, needed for MBTiles access.
target_link_libraries(${FRAMEWORK_NAME} sqlite3)

set(IOS_FRAMEWORK_RESOURCES ${PROJECT_SOURCE_DIR}/platforms/ios/framework/Info.plist)

set_target_properties(${FRAMEWORK_NAME} PROPERTIES
    CLEAN_DIRECT_OUTPUT 1
    FRAMEWORK TRUE
    MACOSX_FRAMEWORK_IDENTIFIER com.mapzen.tangramMap
    MACOSX_FRAMEWORK_INFO_PLIST ${IOS_FRAMEWORK_RESOURCES}
    PUBLIC_HEADER "${FRAMEWORK_HEADERS}"
    RESOURCE "${IOS_FRAMEWORK_RESOURCES}"
    )

set_xcode_property(${FRAMEWORK_NAME} CODE_SIGN_IDENTITY "")
set_xcode_property(${FRAMEWORK_NAME} CODE_SIGNING_REQUIRED "NO")
set_xcode_property(${FRAMEWORK_NAME} CODE_SIGN_ENTITLEMENTS "")
set_xcode_property(${FRAMEWORK_NAME} CODE_SIGNING_ALLOWED "NO")

if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
    set_xcode_property(${FRAMEWORK_NAME} GCC_GENERATE_DEBUGGING_SYMBOLS NO)
    set_xcode_property(${FRAMEWORK_NAME} DEPLOYMENT_POSTPROCESSING YES)
    set_xcode_property(${FRAMEWORK_NAME} COPY_PHASE_STRIP NO)
    set_xcode_property(${FRAMEWORK_NAME} STRIP_INSTALLED_PRODUCT YES)
    set_xcode_property(${FRAMEWORK_NAME} STRIP_STYLE non-global)
    set_xcode_property(${FRAMEWORK_NAME} SEPARATE_STRIP YES)
    set_xcode_property(${FRAMEWORK_NAME} DEAD_CODE_STRIPPING YES)
else()
    set_xcode_property(${FRAMEWORK_NAME} GCC_GENERATE_DEBUGGING_SYMBOLS YES)
endif()

if(${IOS_PLATFORM} STREQUAL "SIMULATOR")
    # properties for simulator architectures
else()
    if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
        set_xcode_property(${FRAMEWORK_NAME} ENABLE_BITCODE "YES")
        set_xcode_property(${FRAMEWORK_NAME} BITCODE_GENERATION_MODE bitcode)
    endif()
endif()

set_xcode_property(${FRAMEWORK_NAME} SUPPORTED_PLATFORMS "iphonesimulator iphoneos")
set_xcode_property(${FRAMEWORK_NAME} ONLY_ACTIVE_ARCH "NO")
set_xcode_property(${FRAMEWORK_NAME} VALID_ARCHS "${IOS_ARCH}")
set_xcode_property(${FRAMEWORK_NAME} ARCHS "${IOS_ARCH}")

# Set RPATH to be within the application /Frameworks directory
set_xcode_property(${FRAMEWORK_NAME} LD_DYLIB_INSTALL_NAME "@rpath/${FRAMEWORK_NAME}.framework/${FRAMEWORK_NAME}")

foreach(_framework ${FRAMEWORKS})
    add_framework(${_framework} ${FRAMEWORK_NAME} ${CMAKE_SYSTEM_FRAMEWORK_PATH})
endforeach()

