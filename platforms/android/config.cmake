add_definitions(-DTANGRAM_ANDROID)

add_library(tangram SHARED
  platforms/common/platform_gl.cpp
  platforms/android/tangram/src/main/cpp/jniExports.cpp
  platforms/android/tangram/src/main/cpp/androidPlatform.cpp
  platforms/android/tangram/src/main/cpp/sqlite3ndk.cpp
)

target_include_directories(tangram
  PRIVATE
  core/deps/SQLiteCpp/sqlite3 # sqlite3ndk.cpp needs sqlite3.h
)

target_link_libraries(tangram
  PRIVATE
  tangram-core
  # android libraries
  android
  atomic
  GLESv2
  log
  z
)
