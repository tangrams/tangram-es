# options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall -fpermissive")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_CXX_FLAGS} -L/opt/vc/lib/")
set(CXX_FLAGS_DEBUG "-g -O0")
set(EXECUTABLE_NAME "tangram")

add_definitions(-DTANGRAM_RPI)

check_unsupported_compiler_version()

get_mapzen_api_key(MAPZEN_API_KEY)
add_definitions(-DMAPZEN_API_KEY="${MAPZEN_API_KEY}")

# load core library
add_subdirectory(${PROJECT_SOURCE_DIR}/core)

add_executable(${EXECUTABLE_NAME}
  ${PROJECT_SOURCE_DIR}/platforms/rpi/src/context.cpp
  ${PROJECT_SOURCE_DIR}/platforms/rpi/src/main.cpp
  ${PROJECT_SOURCE_DIR}/platforms/rpi/src/rpiPlatform.cpp
  ${PROJECT_SOURCE_DIR}/platforms/common/urlClient.cpp
  ${PROJECT_SOURCE_DIR}/platforms/common/platform_gl.cpp
  )

target_include_directories(${EXECUTABLE_NAME}
  PUBLIC
  ${PROJECT_SOURCE_DIR}/platforms/common
  ${PROJECT_SOURCE_DIR}/platforms/rpi/src
  /opt/vc/include/
  /opt/vc/include/interface/vcos/pthreads
  /opt/vc/include/interface/vmcs_host/linux
  )

target_link_libraries(${EXECUTABLE_NAME}
  ${CORE_LIBRARY}
  curl
  GLESv2
  EGL
  bcm_host
  vchiq_arm
  vcos
  rt
  pthread
  )

add_resources(${EXECUTABLE_NAME} "${PROJECT_SOURCE_DIR}/scenes")
