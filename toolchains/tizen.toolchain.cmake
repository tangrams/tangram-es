SET(SDK_ROOT ${TIZEN_SDK})

SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_VERSION 1)

SET(TIZEN_DEVICE OFF)

if (${TIZEN_DEVICE})

  SET(CMAKE_C_COMPILER
    ${SDK_ROOT}/tools/arm-linux-gnueabi-gcc-4.9/bin/arm-linux-gnueabi-gcc)

  SET(CMAKE_CXX_COMPILER
    ${SDK_ROOT}/tools/arm-linux-gnueabi-gcc-4.9/bin/arm-linux-gnueabi-g++)

  SET(CMAKE_FIND_ROOT_PATH
    ${SDK_ROOT}/platforms/tizen-2.4/mobile/rootstraps/mobile-2.4-device.core)

  SET(CMAKE_SYSROOT
    ${SDK_ROOT}/platforms/tizen-2.4/mobile/rootstraps/mobile-2.4-device.core)

else() # Emulator

  SET(CMAKE_C_COMPILER
    ${SDK_ROOT}/tools/i386-linux-gnueabi-gcc-4.9/bin/i386-linux-gnueabi-gcc)

  SET(CMAKE_CXX_COMPILER
    ${SDK_ROOT}/tools/i386-linux-gnueabi-gcc-4.9/bin/i386-linux-gnueabi-g++)

  SET(CMAKE_FIND_ROOT_PATH
    ${SDK_ROOT}/platforms/tizen-2.4/mobile/rootstraps/mobile-2.4-emulator.core)

  SET(CMAKE_SYSROOT
    ${SDK_ROOT}/platforms/tizen-2.4/mobile/rootstraps/mobile-2.4-emulator.core)

  SET(CMAKE_C_FLAGS_INIT "-march=i486")
  SET(CMAKE_CXX_FLAGS_INIT "-march=i486")

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=i486")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=i486")

endif()

SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
