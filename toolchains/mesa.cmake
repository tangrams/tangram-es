include(ExternalProject)

ExternalProject_Add(
  OSMesa

  URL ftp://ftp.freedesktop.org/pub/mesa/mesa-17.0.5.tar.gz

  BUILD_IN_SOURCE 1
  SOURCE_DIR "${CMAKE_BINARY_DIR}/mesa"

  CONFIGURE_COMMAND "${CMAKE_BINARY_DIR}/mesa/configure" --prefix=<INSTALL_DIR> --enable-gallium-osmesa --disable-va --disable-vdpau --disable-gles1 --disable-gles2 --disable-shared-glapi --disable-xvmc --disable-glx --disable-driglx-direct --disable-dri --disable-gbm --disable-egl --with-gallium-drivers=swrast,swr

  BUILD_COMMAND make -j8

  INSTALL_COMMAND make install

  UPDATE_COMMAND ""
  PATCH_COMMAND ""
  TEST_COMMAND ""
)

ExternalProject_Get_Property(OSMesa install_dir)


message(STATUS "MESA ${install_dir} - ${CMAKE_BINARY_DIR}")

ExternalProject_Add_Step(
  OSMesa CopyToBin
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${install_dir}/lib ${CMAKE_BINARY_DIR}/lib}
  DEPENDEES install
)

set(OSMesa_INCLUDE_DIRS "${install_dir}/include")
set(OSMesa_LIBRARIES "${CMAKE_SHARED_LIBRARY_PREFIX}OSMesa${CMAKE_SHARED_LIBRARY_SUFFIX}")
set(OSMesa_LIBRARY_DIR "${install_dir}/lib")


# use, i.e. don't skip the full RPATH for the build tree
SET(CMAKE_SKIP_BUILD_RPATH FALSE)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

SET(CMAKE_INSTALL_RPATH ${CMAKE_BINARY_DIR}/OSMesa-prefix/lib ${CMAKE_INSTALL_RPATH})
link_directories(${CMAKE_BINARY_DIR}/OSMesa-prefix/lib)

#SET(CMAKE_INSTALL_RPATH ${CMAKE_BINARY_DIR}/lib ${CMAKE_INSTALL_RPATH})
#link_directories(${CMAKE_BINARY_DIR}/lib)
