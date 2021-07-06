add_definitions(-DTANGRAM_IOS)

set(TANGRAM_FRAMEWORK_VERSION "0.16.3-dev")
set(TANGRAM_BUNDLE_IDENTIFIER "com.mapzen.TangramMap")

### Configure iOS toolchain.
set(CMAKE_OSX_DEPLOYMENT_TARGET "9.3") # Applies to iOS even though the variable name says OSX.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -fvisibility-inlines-hidden")
execute_process(COMMAND xcrun --sdk iphoneos --show-sdk-version OUTPUT_VARIABLE IOS_SDK_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
# Set the global BITCODE_GENERATION_MODE value to 'bitcode' for Release builds.
# This is for generating full bitcode outside of the "archive" command.
# Set 'marker' for other configs to make debug builds install-able on devices.
set(CMAKE_XCODE_ATTRIBUTE_BITCODE_GENERATION_MODE "$<IF:$<CONFIG:Release>,bitcode,marker>")

if(TANGRAM_XCODE_USE_CCACHE)
  # Following technique described here: https://crascit.com/2016/04/09/using-ccache-with-cmake/
  find_program(CCACHE_PROGRAM ccache REQUIRED)
  # Set up wrapper scripts.
  configure_file("${PROJECT_SOURCE_DIR}/platforms/ios/xcode-ccache-c.in" ccache-c)
  configure_file("${PROJECT_SOURCE_DIR}/platforms/ios/xcode-ccache-cxx.in" ccache-cxx)
  execute_process(COMMAND chmod a+rx "${CMAKE_BINARY_DIR}/ccache-c" "${CMAKE_BINARY_DIR}/ccache-cxx")
  # Set Xcode project attributes to route compilation and linking through our scripts.
  set(CMAKE_XCODE_ATTRIBUTE_CC "${CMAKE_BINARY_DIR}/ccache-c")
  set(CMAKE_XCODE_ATTRIBUTE_CXX "${CMAKE_BINARY_DIR}/ccache-cxx")
  set(CMAKE_XCODE_ATTRIBUTE_LD "${CMAKE_BINARY_DIR}/ccache-c")
  set(CMAKE_XCODE_ATTRIBUTE_LDPLUSPLUS "${CMAKE_BINARY_DIR}/ccache-cxx")
endif()

# Copy necessary workspace settings into a user-specific location in the iOS workspace.
# See platforms/ios/DEVELOPING.md for details.
configure_file(
  ${PROJECT_SOURCE_DIR}/platforms/ios/WorkspaceSettings.xcsettings
  ${PROJECT_SOURCE_DIR}/platforms/ios/Tangram.xcworkspace/xcuserdata/$ENV{USER}.xcuserdatad/WorkspaceSettings.xcsettings
  COPYONLY
)

# Copy a xcconfig file for local signing settings into the demo folder, if it doesn't already exist.
# See platforms/ios/DEVELOPING.md for details.
foreach(LOCAL_XCCONFIG IN ITEMS
  "${PROJECT_SOURCE_DIR}/platforms/ios/demo/Local.xcconfig"
  "${PROJECT_SOURCE_DIR}/platforms/ios/swift/Local.xcconfig"
)
  message(STATUS "LOCAL_XCCONFIG: ${LOCAL_XCCONFIG}")
  if (NOT EXISTS ${LOCAL_XCCONFIG})
    configure_file(
      ${PROJECT_SOURCE_DIR}/platforms/ios/Local.xcconfig.in
      ${LOCAL_XCCONFIG}
      COPYONLY
    )
  endif()
endforeach()

# Configure the API key in the Info.plist for the demo app.
set(NEXTZEN_API_KEY $ENV{NEXTZEN_API_KEY})
configure_file(${PROJECT_SOURCE_DIR}/platforms/ios/demo/Info.plist.in ${PROJECT_BINARY_DIR}/Info.plist)

# Tell SQLiteCpp to not build its own copy of SQLite, we will use the system library instead.
set(SQLITECPP_INTERNAL_SQLITE OFF CACHE BOOL "")
if (IOS_SDK_VERSION VERSION_LESS 11.0)
  set(SQLITE_USE_LEGACY_STRUCT ON CACHE BOOL "")
endif()

# Some 3rd-party dependencies (SQLiteCpp and Freetype) have CMake scripts that
# use find_package() to locate and link libraries that they depend on.
# find_package() successfully sets paths for these library files, but the paths
# are only valid for the *configured* sysroot. For iOS, this means that the 
# library paths use the device SDK, not the simulator. See:
# https://cmake.org/cmake/help/latest/manual/cmake-toolchains.7.html#switching-between-device-and-simulator
# To allow switching between device and simulator builds, we need to use linker
# flags so that the final library path can be determined by the linker. If the
# *_LIBRARY variable is already set when find_package() runs, it will keep the
# current value. This lets us force dependencies to use linker flags instead of
# paths to libraries.
set(ZLIB_LIBRARY "-lz" CACHE STRING "Override zlib library location on iOS")
set(SQLite3_LIBRARY "-lsqlite3" CACHE STRING "Override sqlite3 library location on iOS")

# Headers must be absolute paths for the copy_if_different command on the
# static library target, relative paths cause it to fail with an error.
set(TANGRAM_FRAMEWORK_HEADERS
  ${PROJECT_SOURCE_DIR}/platforms/ios/framework/src/TangramMap.h
  ${PROJECT_SOURCE_DIR}/platforms/ios/framework/src/TGCameraPosition.h
  ${PROJECT_SOURCE_DIR}/platforms/ios/framework/src/TGExport.h
  ${PROJECT_SOURCE_DIR}/platforms/ios/framework/src/TGGeometry.h
  ${PROJECT_SOURCE_DIR}/platforms/ios/framework/src/TGGeoPolygon.h
  ${PROJECT_SOURCE_DIR}/platforms/ios/framework/src/TGGeoPolyline.h
  ${PROJECT_SOURCE_DIR}/platforms/ios/framework/src/TGLabelPickResult.h
  ${PROJECT_SOURCE_DIR}/platforms/ios/framework/src/TGMapData.h
  ${PROJECT_SOURCE_DIR}/platforms/ios/framework/src/TGMapFeature.h
  ${PROJECT_SOURCE_DIR}/platforms/ios/framework/src/TGMapView.h
  ${PROJECT_SOURCE_DIR}/platforms/ios/framework/src/TGMapViewDelegate.h
  ${PROJECT_SOURCE_DIR}/platforms/ios/framework/src/TGMarker.h
  ${PROJECT_SOURCE_DIR}/platforms/ios/framework/src/TGMarkerPickResult.h
  ${PROJECT_SOURCE_DIR}/platforms/ios/framework/src/TGRecognizerDelegate.h
  ${PROJECT_SOURCE_DIR}/platforms/ios/framework/src/TGSceneUpdate.h
  ${PROJECT_SOURCE_DIR}/platforms/ios/framework/src/TGTypes.h
  ${PROJECT_SOURCE_DIR}/platforms/ios/framework/src/TGURLHandler.h
)

set(TANGRAM_FRAMEWORK_SOURCES
  ${TANGRAM_FRAMEWORK_HEADERS}
  platforms/common/appleAllowedFonts.h
  platforms/common/appleAllowedFonts.mm
  platforms/common/platform_gl.cpp
  platforms/ios/framework/src/iosPlatform.h
  platforms/ios/framework/src/iosPlatform.mm
  platforms/ios/framework/src/TGCameraPosition+Internal.h
  platforms/ios/framework/src/TGCameraPosition.mm
  platforms/ios/framework/src/TGGeoPolygon.m
  platforms/ios/framework/src/TGGeoPolyline.m
  platforms/ios/framework/src/TGLabelPickResult.mm
  platforms/ios/framework/src/TGLabelPickResult+Internal.h
  platforms/ios/framework/src/TGMapData+Internal.h
  platforms/ios/framework/src/TGMapData.mm
  platforms/ios/framework/src/TGMapFeature+Internal.h
  platforms/ios/framework/src/TGMapFeature.m
  platforms/ios/framework/src/TGMapView.mm
  platforms/ios/framework/src/TGMapView+Internal.h
  platforms/ios/framework/src/TGMarker.mm
  platforms/ios/framework/src/TGMarker+Internal.h
  platforms/ios/framework/src/TGMarkerPickResult.mm
  platforms/ios/framework/src/TGMarkerPickResult+Internal.h
  platforms/ios/framework/src/TGSceneUpdate.mm
  platforms/ios/framework/src/TGTypes+Internal.h
  platforms/ios/framework/src/TGTypes.mm
  platforms/ios/framework/src/TGURLHandler.mm
)

### Configure dynamic framework build target. 

add_library(TangramMap SHARED
  ${TANGRAM_FRAMEWORK_SOURCES}
)

target_link_libraries(TangramMap PRIVATE
  tangram-core
  # Frameworks: use quotes so "-framework X" is treated as a single linker flag.
  "-framework CoreFoundation"
  "-framework CoreGraphics"
  "-framework CoreLocation"
  "-framework CoreText"
  "-framework GLKit"
  "-framework OpenGLES"
  "-framework QuartzCore"
  "-framework UIKit"
)

target_include_directories(TangramMap PRIVATE
  platforms/common
)

set_target_properties(TangramMap PROPERTIES
  FRAMEWORK TRUE
  PUBLIC_HEADER "${TANGRAM_FRAMEWORK_HEADERS}"
  MACOSX_FRAMEWORK_INFO_PLIST "${PROJECT_SOURCE_DIR}/platforms/ios/framework/Info.plist"
  XCODE_ATTRIBUTE_DEFINES_MODULE "YES"
  # This property is necessary for downstream consumers of the framework. Set it here for Xcode to
  # populate the 'Bundle Identifier' fields and substitute the placeholder value in our Info.plist.
  XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${TANGRAM_BUNDLE_IDENTIFIER}"
  # This ensures that the dynamic linker loads the framework from its embedded path in an app, not
  # from the location it was originally built.
  XCODE_ATTRIBUTE_INSTALL_PATH "@rpath"
  # Ensure that archives are built for distribution.
  XCODE_ATTRIBUTE_BUILD_LIBRARY_FOR_DISTRIBUTION "YES"
)
# Other properties that are common to dynamic and static framework targets are set below.

### Configure static library build target.

add_library(tangram-static STATIC
  ${TANGRAM_FRAMEWORK_SOURCES}
)

target_link_libraries(tangram-static PRIVATE
  tangram-core
  # Frameworks: use quotes so "-framework X" is treated as a single linker flag.
  "-framework CoreFoundation"
  "-framework CoreGraphics"
  "-framework CoreLocation"
  "-framework CoreText"
  "-framework GLKit"
  "-framework OpenGLES"
  "-framework QuartzCore"
  "-framework UIKit"
)

target_include_directories(tangram-static PRIVATE
  platforms/common
)

# To produce a distributable version of our iOS SDK as a static library, we
# need to collect all the symbols that will be needed in a final, linked
# executable. CMake normally makes these symbols available by propagating link
# flags from dependencies, but users who aren't building the SDK from source
# won't have the dependency libraries in a build tree. We solve this by pre-
# linking all of the dependency libraries that we would normally pass as flags.

# Here we manually list the library files for all of our compiled dependencies.
# I haven't found a way to get a full, recursive library list from CMake
# so this list will need to be updated when any new library dependencies are
# added. Note the quotes: this is needed to not make it a "list", which CMake
# delimits with semicolons. Xcode expects a space-delimited list.
set(TANGRAM_STATIC_DEPENDENCIES "\
  $<TARGET_FILE:tangram-core>
  $<TARGET_FILE:css-color-parser-cpp>
  $<TARGET_FILE:yaml-cpp>
  $<TARGET_FILE:alfons>
  $<TARGET_FILE:linebreak>
  $<TARGET_FILE:harfbuzz>
  $<TARGET_FILE:freetype>
  $<TARGET_FILE:icucommon>
  $<TARGET_FILE:double-conversion>
  $<TARGET_FILE:miniz>
  "
)
if(NOT TANGRAM_USE_JSCORE)
  set(TANGRAM_STATIC_DEPENDENCIES "${TANGRAM_STATIC_DEPENDENCIES} $<TARGET_FILE:duktape>")
endif()
if(TANGRAM_MBTILES_DATASOURCE)
  set(TANGRAM_STATIC_DEPENDENCIES "${TANGRAM_STATIC_DEPENDENCIES} $<TARGET_FILE:SQLiteCpp>")
endif()

set_target_properties(tangram-static PROPERTIES
  # The Xcode settings below are to pre-link our static libraries into a single
  # archive. Xcode will take the objects from this target and from all of the
  # pre-link libraries, combine them, and resolve the symbols into one "master"
  # object file before outputting an archive.
  XCODE_ATTRIBUTE_GENERATE_MASTER_OBJECT_FILE "YES"
  XCODE_ATTRIBUTE_PRELINK_LIBS "${TANGRAM_STATIC_DEPENDENCIES}"
)

# Set properties common between dynamic and static framework targets.
set_target_properties(TangramMap tangram-static PROPERTIES
  XCODE_GENERATE_SCHEME TRUE
  XCODE_ATTRIBUTE_CURRENT_PROJECT_VERSION "${TANGRAM_FRAMEWORK_VERSION}"
  XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC "YES"
  XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++14"
  XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++"
  XCODE_ATTRIBUTE_GCC_TREAT_WARNINGS_AS_ERRORS "YES"
  # Generate dsym for all build types to ensure symbols are available in profiling.
  XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "YES"
  # Ensure that archives are built for distribution.
  XCODE_ATTRIBUTE_BUILD_LIBRARY_FOR_DISTRIBUTION "YES"
)

# Copy the framework headers into a directory in the build folder, for use by
# the static demo app build and for distribution.
add_custom_command(TARGET tangram-static POST_BUILD
  COMMAND
  ${CMAKE_COMMAND} -E make_directory
  "${PROJECT_BINARY_DIR}/Headers/"
  COMMAND
  ${CMAKE_COMMAND} -E copy_if_different
  ${TANGRAM_FRAMEWORK_HEADERS}
  "${PROJECT_BINARY_DIR}/Headers/"
)
