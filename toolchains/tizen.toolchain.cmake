set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_VERSION 1)

message(STATUS "Tizen SDK path: ${TIZEN_SDK}, Target: ${TIZEN_DEVICE}")

if (${TIZEN_DEVICE})

  set(CMAKE_C_COMPILER ${TIZEN_SDK}/tools/arm-linux-gnueabi-gcc-4.9/bin/arm-linux-gnueabi-gcc)
  set(CMAKE_CXX_COMPILER ${TIZEN_SDK}/tools/arm-linux-gnueabi-gcc-4.9/bin/arm-linux-gnueabi-g++)
  set(CMAKE_FIND_ROOT_PATH ${TIZEN_SDK}/platforms/tizen-2.3.1/wearable/rootstraps/wearable-2.3.1-device.core)

  # set(CMAKE_SYSROOT  ${TIZEN_SDK}/platforms/tizen-2.3.1/wearable/rootstraps/wearable-2.3.1-device.core)
  set(CMAKE_SYSROOT ${TIZEN_SYSROOT})

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
  set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

else() # Emulator

  set(CMAKE_C_COMPILER ${TIZEN_SDK}/tools/i386-linux-gnueabi-gcc-4.9/bin/i386-linux-gnueabi-gcc)
  set(CMAKE_CXX_COMPILER ${TIZEN_SDK}/tools/i386-linux-gnueabi-gcc-4.9/bin/i386-linux-gnueabi-g++)
  set(CMAKE_FIND_ROOT_PATH ${TIZEN_SDK}/platforms/tizen-2.3.1/wearable/rootstraps/wearable-2.3.1-emulator.core)

  # set(TIZEN_SYSROOT ${TIZEN_SDK}/platforms/tizen-2.3.1/wearable/rootstraps/wearable-2.3.1-emulator.core)
  set(CMAKE_SYSROOT ${TIZEN_SYSROOT})

  # set(CMAKE_C_FLAGS_INIT "-march=i486")
  # set(CMAKE_CXX_FLAGS_INIT "-march=i486")
  # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=i486")
  # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=i486")

endif()

message(STATUS "SYSROOT: ${CMAKE_SYSROOT}")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
