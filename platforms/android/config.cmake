add_definitions(-DTANGRAM_ANDROID)

if ($ENV{CIRCLECI})
  # Force Ninja on CircleCI to use a specific number of concurrent jobs.
  # Otherwise it guesses concurrency based on the physical CPU (which has a lot of cores!) and runs out of memory.
  message(STATUS "Limiting concurrent jobs due to CI environment.")
  set_property(GLOBAL APPEND PROPERTY JOB_POOLS compile_pool=4 link_pool=2)
  set(CMAKE_JOB_POOL_COMPILE compile_pool)
  set(CMAKE_JOB_POOL_LINK link_pool)
endif()

add_library(tangram SHARED
  platforms/common/platform_gl.cpp
  platforms/android/tangram/src/main/cpp/jniExports.cpp
  platforms/android/tangram/src/main/cpp/androidPlatform.cpp
)

if(TANGRAM_MBTILES_DATASOURCE)
  target_sources(tangram PRIVATE platforms/android/tangram/src/main/cpp/sqlite3ndk.cpp)
  target_include_directories(tangram PRIVATE core/deps/SQLiteCpp/sqlite3) # sqlite3ndk.cpp needs sqlite3.h
  target_compile_definitions(tangram PRIVATE TANGRAM_MBTILES_DATASOURCE=1)
endif()

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
