set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_VERSION 1)

message(STATUS "Tizen SDK path: ${TIZEN_SDK}, Device: ${TIZEN_DEVICE}")

set(CMAKE_C_COMPILER_WORKS TRUE)
set(CMAKE_CXX_COMPILER_WORKS TRUE)

if (${TIZEN_DEVICE})
  set(CMAKE_C_COMPILER ${TIZEN_SDK}/tools/arm-linux-gnueabi-gcc-4.9/bin/arm-linux-gnueabi-gcc)
  set(CMAKE_CXX_COMPILER ${TIZEN_SDK}/tools/arm-linux-gnueabi-gcc-4.9/bin/arm-linux-gnueabi-g++)
else() # Emulator
  set(CMAKE_C_COMPILER ${TIZEN_SDK}/tools/i386-linux-gnueabi-gcc-4.9/bin/i386-linux-gnueabi-gcc)
  set(CMAKE_CXX_COMPILER ${TIZEN_SDK}/tools/i386-linux-gnueabi-gcc-4.9/bin/i386-linux-gnueabi-g++)
endif()

set(CMAKE_FIND_ROOT_PATH ${TIZEN_SYSROOT})
set(CMAKE_SYSROOT ${TIZEN_SYSROOT})
include_directories(SYSTEM
  "${TIZEN_SYSROOT}/usr/include/libxml2"
  "${TIZEN_SYSROOT}/usr/include"
  "${TIZEN_SYSROOT}/usr/include/appcore-agent"
  "${TIZEN_SYSROOT}/usr/include/appfw"
  "${TIZEN_SYSROOT}/usr/include/attach-panel"
  "${TIZEN_SYSROOT}/usr/include/badge"
  "${TIZEN_SYSROOT}/usr/include/base"
  "${TIZEN_SYSROOT}/usr/include/cairo"
  "${TIZEN_SYSROOT}/usr/include/calendar-service2"
  "${TIZEN_SYSROOT}/usr/include/chromium-ewk"
  "${TIZEN_SYSROOT}/usr/include/ckm"
  "${TIZEN_SYSROOT}/usr/include/contacts-svc"
  "${TIZEN_SYSROOT}/usr/include/content"
  "${TIZEN_SYSROOT}/usr/include/context-service"
  "${TIZEN_SYSROOT}/usr/include/dali"
  "${TIZEN_SYSROOT}/usr/include/dali-toolkit"
  "${TIZEN_SYSROOT}/usr/include/dbus-1.0"
  "${TIZEN_SYSROOT}/usr/include/device"
  "${TIZEN_SYSROOT}/usr/include/dlog"
  "${TIZEN_SYSROOT}/usr/include/ecore-1"
  "${TIZEN_SYSROOT}/usr/include/ecore-buffer-1"
  "${TIZEN_SYSROOT}/usr/include/ecore-con-1"
  "${TIZEN_SYSROOT}/usr/include/ecore-evas-1"
  "${TIZEN_SYSROOT}/usr/include/ecore-file-1"
  "${TIZEN_SYSROOT}/usr/include/ecore-imf-1"
  "${TIZEN_SYSROOT}/usr/include/ecore-imf-evas-1"
  "${TIZEN_SYSROOT}/usr/include/ecore-input-1"
  "${TIZEN_SYSROOT}/usr/include/ecore-input-evas-1"
  "${TIZEN_SYSROOT}/usr/include/ecore-ipc-1"
  "${TIZEN_SYSROOT}/usr/include/ector-1"
  "${TIZEN_SYSROOT}/usr/include/e_dbus-1"
  "${TIZEN_SYSROOT}/usr/include/edje-1"
  "${TIZEN_SYSROOT}/usr/include/eet-1"
  "${TIZEN_SYSROOT}/usr/include/efl-1"
  "${TIZEN_SYSROOT}/usr/include/efl-extension"
  "${TIZEN_SYSROOT}/usr/include/efreet-1"
  "${TIZEN_SYSROOT}/usr/include/eina-1"
  "${TIZEN_SYSROOT}/usr/include/eina-1/eina"
  "${TIZEN_SYSROOT}/usr/include/eio-1"
  "${TIZEN_SYSROOT}/usr/include/eldbus-1"
  "${TIZEN_SYSROOT}/usr/include/elementary-1"
  "${TIZEN_SYSROOT}/usr/include/embryo-1"
  "${TIZEN_SYSROOT}/usr/include/emile-1"
  "${TIZEN_SYSROOT}/usr/include/eo-1"
  "${TIZEN_SYSROOT}/usr/include/eom"
  "${TIZEN_SYSROOT}/usr/include/ethumb-1"
  "${TIZEN_SYSROOT}/usr/include/ethumb-client-1"
  "${TIZEN_SYSROOT}/usr/include/evas-1"
  "${TIZEN_SYSROOT}/usr/include/feedback"
  "${TIZEN_SYSROOT}/usr/include/fontconfig"
  "${TIZEN_SYSROOT}/usr/include/freetype2"
  "${TIZEN_SYSROOT}/usr/include/geofence"
  "${TIZEN_SYSROOT}/usr/include/gio-unix-2.0"
  "${TIZEN_SYSROOT}/usr/include/glib-2.0"
  "${TIZEN_SYSROOT}/usr/include/harfbuzz"
  "${TIZEN_SYSROOT}/usr/include/iotcon"
  "${TIZEN_SYSROOT}/usr/include/json-glib-1.0"
  "${TIZEN_SYSROOT}/usr/include/location"
  "${TIZEN_SYSROOT}/usr/include/maps"
  "${TIZEN_SYSROOT}/usr/include/media"
  "${TIZEN_SYSROOT}/usr/include/media-content"
  "${TIZEN_SYSROOT}/usr/include/messaging"
  "${TIZEN_SYSROOT}/usr/include/metadata-editor"
  "${TIZEN_SYSROOT}/usr/include/minicontrol"
  "${TIZEN_SYSROOT}/usr/include/minizip"
  "${TIZEN_SYSROOT}/usr/include/network"
  "${TIZEN_SYSROOT}/usr/include/notification"
  "${TIZEN_SYSROOT}/usr/include/nsd/"
  "${TIZEN_SYSROOT}/usr/include/phonenumber-utils"
  "${TIZEN_SYSROOT}/usr/include/sensor"
  "${TIZEN_SYSROOT}/usr/include/service-adaptor"
  "${TIZEN_SYSROOT}/usr/include/shortcut"
  "${TIZEN_SYSROOT}/usr/include/storage"
  "${TIZEN_SYSROOT}/usr/include/system"
  "${TIZEN_SYSROOT}/usr/include/telephony"
  "${TIZEN_SYSROOT}/usr/include/ui"
  "${TIZEN_SYSROOT}/usr/include/web"
  "${TIZEN_SYSROOT}/usr/include/widget_service"
  "${TIZEN_SYSROOT}/usr/include/widget_viewer_evas"
  "${TIZEN_SYSROOT}/usr/include/wifi-direct"
  "${TIZEN_SYSROOT}/usr/lib/dbus-1.0/include"
  "${TIZEN_SYSROOT}/usr/lib/glib-2.0/include")

message(STATUS "SYSROOT: ${CMAKE_SYSROOT}")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
