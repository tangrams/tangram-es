### Worker

include(${CMAKE_CURRENT_SOURCE_DIR}/prime_server.cmake)

# use, i.e. don't skip the full RPATH for the build tree
SET(CMAKE_SKIP_BUILD_RPATH  FALSE)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
SET(CMAKE_INSTALL_RPATH ${CMAKE_BINARY_DIR}/lib ${CMAKE_INSTALL_RPATH})

# the RPATH to be used when installing, but only if it's not a system directory
# LIST(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
# IF("${isSystemDir}" STREQUAL "-1")
#    SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
# ENDIF("${isSystemDir}" STREQUAL "-1")

# if (USE_SYSTEM_OSMESA_LIBS)
#   include(FindPkgConfig)
#   pkg_check_modules(OSMesa REQUIRED osmesa)
# else ()
#   include(${PROJECT_SOURCE_DIR}/toolchains/mesa.cmake)
# endif()


add_executable(paparazzi_worker
  src/worker.cpp
  src/paparazzi.cpp
  src/stb_image.cpp)

target_include_directories(paparazzi_worker
  PUBLIC
  src/tools
  ${PROJECT_SOURCE_DIR}/platforms/common
  ${PROJECT_SOURCE_DIR}/platforms/linux/src
  ${OSMesa_INCLUDE_DIRS}
  ${prime_server_INCLUDE_DIRS})

target_link_libraries(paparazzi_worker
  core
  headless_context
  -lpthread
  -lOSMesa
  -lGL
  ${prime_server_LIBRARIES}
  ${PRIME_SERVER_DEPS_LIBRARIES}
  )

target_compile_definitions(paparazzi_worker
    PRIVATE
    PLATFORM_HEADLESS=1)

target_link_libraries(paparazzi_worker
  platform_linux)

add_dependencies(paparazzi_worker external-prime_server)

### Proxy

add_executable(paparazzi_proxy
  src/proxy.cpp)

target_include_directories(paparazzi_proxy
  PUBLIC
  ${prime_server_INCLUDE_DIRS})

target_link_libraries(paparazzi_proxy
  ${prime_server_LIBRARIES}
  ${PRIME_SERVER_DEPS_LIBRARIES}
  )

add_dependencies(paparazzi_proxy external-prime_server)
