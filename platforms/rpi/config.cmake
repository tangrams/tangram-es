add_definitions(-DTANGRAM_RPI)

check_unsupported_compiler_version()

get_nextzen_api_key(NEXTZEN_API_KEY)
add_definitions(-DNEXTZEN_API_KEY="${NEXTZEN_API_KEY}")

# System font config
include(FindPkgConfig)
pkg_check_modules(FONTCONFIG REQUIRED "fontconfig")

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
  pthread
  rt
  /opt/vc/lib/libbcm_host.so
  /opt/vc/lib/libbrcmEGL.so
  /opt/vc/lib/libbrcmGLESv2.so
  /opt/vc/lib/libvchiq_arm.so
  /opt/vc/lib/libvcos.so
)

target_compile_options(tangram
  PRIVATE
  -std=c++14
  -Wall
  -fpermissive
)

add_resources(tangram "${PROJECT_SOURCE_DIR}/scenes" "res")
