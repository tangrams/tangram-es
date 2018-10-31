add_definitions(-DTANGRAM_ANDROID)

if (CIRCLECI)
  # Force Ninja on CircleCI to use only 4 concurrent jobs.
  # Otherwise it guesses concurrency based on the physical CPU (which has a lot of cores!) and runs out of memory.
  #set(CMAKE_MAKE_PROGRAM "${CMAKE_MAKE_PROGRAM} -j 4")

  set(CMAKE_JOB_POOL_COMPILE:STRING compile)
  set(CMAKE_JOB_POOL_LINK:STRING link)
  set(CMAKE_JOB_POOLS:STRING compile=16;link=2)
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
