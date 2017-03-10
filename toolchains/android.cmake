add_definitions(-DPLATFORM_ANDROID)

# load core library
add_subdirectory(${PROJECT_SOURCE_DIR}/core)

set(ANDROID_PROJECT_DIR ${PROJECT_SOURCE_DIR}/platforms/android/tangram)

set(LIB_NAME tangram) # in order to have libtangram.so

add_library(${LIB_NAME} SHARED
  ${CMAKE_SOURCE_DIR}/platforms/common/platform_gl.cpp
  ${CMAKE_SOURCE_DIR}/platforms/android/tangram/src/main/cpp/jniExports.cpp
  ${CMAKE_SOURCE_DIR}/platforms/android/tangram/src/main/cpp/platform_android.cpp
  ${CMAKE_SOURCE_DIR}/platforms/android/tangram/src/main/cpp/sqlite3ndk.cpp)

target_include_directories(${LIB_NAME} PUBLIC
  ${CMAKE_SOURCE_DIR}/core/deps/SQLiteCpp/sqlite3) # sqlite3ndk.cpp needs sqlite3.h

target_link_libraries(${LIB_NAME}
  PUBLIC
  ${CORE_LIBRARY}
  # android libaries
  GLESv2 log z atomic android)

