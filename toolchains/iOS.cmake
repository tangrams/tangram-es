include(${CMAKE_SOURCE_DIR}/toolchains/iOS.toolchain.cmake)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
set(ARCH "armv7 armv7s arm64 i386 x86_64")
set(SUPPORTED_PLATFORMS "iphonesimulator iphoneos")
set(TANGRAM_FRAMEWORK ${PROJECT_SOURCE_DIR}/${TANGRAM_FRAMEWORK})
set(EXECUTABLE_NAME "tangram")
set(FRAMEWORKS CoreGraphics CoreFoundation QuartzCore UIKit OpenGLES Security CFNetwork GLKit)

message(STATUS "Linking with Tangram Framework " ${TANGRAM_FRAMEWORK})
message(STATUS "Building for architectures " ${ARCH})

# ios source files
set(IOS_EXTENSIONS_FILES *.mm *.cpp *.m)
foreach(_ext ${IOS_EXTENSIONS_FILES})
    find_sources_and_include_directories(
        ${PROJECT_SOURCE_DIR}/platforms/ios/demo/src/*.h
        ${PROJECT_SOURCE_DIR}/platforms/ios/demo/src/${_ext})
endforeach()

add_bundle_resources(IOS_DEMO_RESOURCES "${PROJECT_SOURCE_DIR}/platforms/ios/demo/resources/" "Resources")
file(GLOB_RECURSE IOS_DEMO_SOURCES ${PROJECT_SOURCE_DIR}/platforms/ios/demo/src/**)

add_executable(${EXECUTABLE_NAME} MACOSX_BUNDLE ${HEADERS} ${SOURCES}
    ${RESOURCES} ${IOS_DEMO_RESOURCES} ${TANGRAM_FRAMEWORK})

target_link_libraries(${EXECUTABLE_NAME} ${TANGRAM_FRAMEWORK})

# setting xcode properties
set_target_properties(${EXECUTABLE_NAME} PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${PROJECT_SOURCE_DIR}/platforms/ios/demo/Info.plist
    MACOSX_FRAMEWORK_IDENTIFIER "com.mapzen.\${PRODUCT_NAME:{EXECUTABLE_NAME}}"
    RESOURCE "${IOS_DEMO_RESOURCES}")

set_source_files_properties(${TANGRAM_FRAMEWORK} PROPERTIES
    MACOSX_PACKAGE_LOCATION ${EXECUTABLE_NAME}.app/Frameworks)

if(NOT ${CMAKE_BUILD_TYPE} STREQUAL "Release")
    set_xcode_property(${EXECUTABLE_NAME} GCC_GENERATE_DEBUGGING_SYMBOLS YES)
endif()

set_xcode_property(${EXECUTABLE_NAME} CODE_SIGN_IDENTITY "iPhone Developer")
set_xcode_property(${EXECUTABLE_NAME} SUPPORTED_PLATFORMS ${SUPPORTED_PLATFORMS})
set_xcode_property(${EXECUTABLE_NAME} ONLY_ACTIVE_ARCH "YES")
set_xcode_property(${EXECUTABLE_NAME} VALID_ARCHS "${ARCH}")
set_xcode_property(${EXECUTABLE_NAME} TARGETED_DEVICE_FAMILY "1,2")
set_xcode_property(${EXECUTABLE_NAME} LD_RUNPATH_SEARCH_PATHS "@executable_path/Frameworks")

foreach(_framework ${FRAMEWORKS})
    add_framework(${_framework} ${EXECUTABLE_NAME} ${CMAKE_SYSTEM_FRAMEWORK_PATH})
endforeach()
