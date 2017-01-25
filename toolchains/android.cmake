add_definitions(-DPLATFORM_ANDROID)

# build external dependencies
add_subdirectory(${PROJECT_SOURCE_DIR}/external)

# load core library
add_subdirectory(${PROJECT_SOURCE_DIR}/core)

set(ANDROID_PROJECT_DIR ${PROJECT_SOURCE_DIR}/android/tangram)

set(LIB_NAME tangram) # in order to have libtangram.so

set(NAO_MAP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../AndroidApps/AndroidStudioProjects/NAO/naomap/)
add_library(${LIB_NAME} SHARED
  ${CMAKE_CURRENT_SOURCE_DIR}/core/common/platform_gl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../NAOMapLib/code/ITangramHelper.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../NAOMapLib/code/IUrlCallback.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../NAOMapLib/code/PlatformTangramImpl.hpp
  
  #${CMAKE_CURRENT_SOURCE_DIR}/android/tangram/src/main/cpp/jniExports.cpp
  #${CMAKE_CURRENT_SOURCE_DIR}/android/tangram/src/main/cpp/platform_android.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/android/tangram/src/main/cpp/platform_android_nao.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/android/tangram/src/main/cpp/djinni_main.cpp
  
  ${NAO_MAP_DIR}src/main/jni/NativePlatformTangramImpl.cpp
  ${NAO_MAP_DIR}src/main/jni/NativeITangramHelper.cpp
  ${NAO_MAP_DIR}src/main/jni/NativeIUrlCallback.cpp
  )

 
target_include_directories(${LIB_NAME} PRIVATE 
	${CMAKE_CURRENT_SOURCE_DIR}/../NAOMapLib/code/)
  
target_link_libraries(${LIB_NAME}
  PUBLIC
  ${CORE_LIBRARY}
  # android libaries
  GLESv2 log z atomic android djinni)

