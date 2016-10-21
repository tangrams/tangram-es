include(${CMAKE_SOURCE_DIR}/toolchains/iOS.toolchain.cmake)

set(EXECUTABLE_NAME "Tangram-iOS-Demo-App")
set(TANGRAM_FRAMEWORK ${PROJECT_SOURCE_DIR}/build/ios-framework-universal/${CMAKE_BUILD_TYPE}/TangramMap.framework)
set(FRAMEWORKS CoreGraphics CoreFoundation QuartzCore UIKit OpenGLES Security CFNetwork GLKit)

# ios source files
set(IOS_EXTENSIONS_FILES *.mm *.cpp *.m)
foreach(_ext ${IOS_EXTENSIONS_FILES})
    find_sources_and_include_directories(
        ${PROJECT_SOURCE_DIR}/ios/demo/src/*.h
        ${PROJECT_SOURCE_DIR}/ios/demo/src/${_ext})
endforeach()

add_bundle_resources(IOS_DEMO_RESOURCES "${PROJECT_SOURCE_DIR}/ios/demo/resources/" "Resources")
file(GLOB_RECURSE IOS_DEMO_SOURCES ${PROJECT_SOURCE_DIR}/ios/demo/src/**)
#source_group("Resources" FILES ${IOS_DEMO_RESOURCES})

add_executable(${EXECUTABLE_NAME} MACOSX_BUNDLE ${HEADERS} ${SOURCES}
    ${RESOURCES} ${IOS_DEMO_RESOURCES} ${TANGRAM_FRAMEWORK})

target_link_libraries(${EXECUTABLE_NAME} ${TANGRAM_FRAMEWORK})

# setting xcode properties
set_target_properties(${EXECUTABLE_NAME} PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${PROJECT_SOURCE_DIR}/ios/demo/resources/tangram-Info.plist
    MACOSX_FRAMEWORK_IDENTIFIER "com.mapzen.\${PRODUCT_NAME:{EXECUTABLE_NAME}}"
    RESOURCE "${IOS_DEMO_RESOURCES}")

set_source_files_properties(${TANGRAM_FRAMEWORK} PROPERTIES
    MACOSX_PACKAGE_LOCATION ${EXECUTABLE_NAME}.app/Frameworks)

if(NOT ${CMAKE_BUILD_TYPE} STREQUAL "Release")
    set_xcode_property(${EXECUTABLE_NAME} GCC_GENERATE_DEBUGGING_SYMBOLS YES)
endif()

set_xcode_property(${EXECUTABLE_NAME} SUPPORTED_PLATFORMS "iphonesimulator iphoneos")
set_xcode_property(${EXECUTABLE_NAME} ONLY_ACTIVE_ARCH "YES")
set_xcode_property(${EXECUTABLE_NAME} VALID_ARCHS "${ARCH}")
set_xcode_property(${EXECUTABLE_NAME} TARGETED_DEVICE_FAMILY "1,2")
set_xcode_property(${EXECUTABLE_NAME} LD_RUNPATH_SEARCH_PATHS "@executable_path/Frameworks")

foreach(_framework ${FRAMEWORKS})
    add_framework(${_framework} ${EXECUTABLE_NAME} ${CMAKE_SYSTEM_FRAMEWORK_PATH})
endforeach()
