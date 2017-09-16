add_definitions(-DTANGRAM_ANDROID)

# load core library
add_subdirectory(${PROJECT_SOURCE_DIR}/core)

set(ANDROID_PROJECT_DIR ${PROJECT_SOURCE_DIR}/platforms/android/tangram)

set(LIB_NAME tangram) # in order to have libtangram.so

add_library(${LIB_NAME} SHARED
  ${PROJECT_SOURCE_DIR}/platforms/common/platform_gl.cpp
  ${PROJECT_SOURCE_DIR}/platforms/android/tangram/src/main/cpp/jniExports.cpp
  ${PROJECT_SOURCE_DIR}/platforms/android/tangram/src/main/cpp/androidPlatform.cpp
  ${PROJECT_SOURCE_DIR}/platforms/android/tangram/src/main/cpp/sqlite3ndk.cpp)

target_include_directories(${LIB_NAME} PUBLIC
  ${PROJECT_SOURCE_DIR}/core/deps/SQLiteCpp/sqlite3) # sqlite3ndk.cpp needs sqlite3.h

target_link_libraries(${LIB_NAME}
  PUBLIC
  ${CORE_LIBRARY}
  # android libraries
  GLESv2 log z atomic android)
