# options
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_CXX_FLAGS} -L/opt/vc/lib/")
set(CXX_FLAGS_DEBUG "-g -O0")

add_definitions(-DTANGRAM_RPI)

check_unsupported_compiler_version()

get_nextzen_api_key(NEXTZEN_API_KEY)
add_definitions(-DNEXTZEN_API_KEY="${NEXTZEN_API_KEY}")

# System font config
include(FindPkgConfig)
pkg_check_modules(FONTCONFIG REQUIRED "fontconfig")

add_subdirectory(platforms/common/duktape)

add_executable(tangram
  platforms/rpi/src/context.cpp
  platforms/rpi/src/main.cpp
  platforms/rpi/src/rpiPlatform.cpp
  platforms/common/urlClient.cpp
  platforms/common/linuxSystemFontHelper.cpp
  platforms/common/platform_gl.cpp
)

target_include_directories(tangram
  PRIVATE
  platforms/common
  platforms/rpi/src
  ${FONTCONFIG_INCLUDE_DIRS}
  /opt/vc/include/
  /opt/vc/include/interface/vcos/pthreads
  /opt/vc/include/interface/vmcs_host/linux
)

target_link_libraries(tangram
  PRIVATE
  tangram-core
  ${FONTCONFIG_LDFLAGS}
  curl
  EGL
  GLESv2
  pthread
  bcm_host
  vchiq_arm
  vcos
  rt
)

target_compile_options(tangram
  PRIVATE
  -std=c++14
  -Wall
  -fpermissive
)

add_resources(tangram "${PROJECT_SOURCE_DIR}/scenes")
