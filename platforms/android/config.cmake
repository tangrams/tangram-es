add_definitions(-DTANGRAM_ANDROID)

if (CIRCLECI)
  # Force Ninja on CircleCI to use only 4 concurrent jobs.
  # Otherwise it guesses concurrency based on the physical CPU (which has a lot of cores!) and runs out of memory.
  set(CMAKE_MAKE_PROGRAM "${CMAKE_MAKE_PROGRAM} -j 4")
endif(CIRCLECI)

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
  jnigraphics
)
