include(FindPkgConfig)

pkg_check_modules(PRIME_SERVER_DEPS REQUIRED libzmq>=4.1.3 libczmq>=3.0 libcurl>=7.22.0)

include(ExternalProject)

ExternalProject_Add(
  external-prime_server

  GIT_REPOSITORY "https://github.com/kevinkreiser/prime_server.git"

  UPDATE_COMMAND ""
  PATCH_COMMAND ""

  BUILD_IN_SOURCE 1
  SOURCE_DIR "${CMAKE_BINARY_DIR}/prime_server"
  #BINARY_DIR "${CMAKE_BINARY_DIR}/prime_server-build"

  CONFIGURE_COMMAND "${CMAKE_BINARY_DIR}/prime_server/autogen.sh" && "${CMAKE_BINARY_DIR}/prime_server/configure" --prefix=<INSTALL_DIR>

  BUILD_COMMAND make

  TEST_COMMAND ""

  INSTALL_COMMAND make install
)

set(external-prime_server_CXXFLAGS "-lpthread")

ExternalProject_Get_Property(external-prime_server install_dir)

ExternalProject_Add_Step(
  external-prime_server CopyToBin
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${install_dir}/bin ${CMAKE_BINARY_DIR}/bin
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${install_dir}/lib ${CMAKE_BINARY_DIR}/lib
  DEPENDEES install
)

set(prime_server_INCLUDE_DIRS "${install_dir}/include")
set(prime_server_LIBRARIES "${CMAKE_SHARED_LIBRARY_PREFIX}prime_server${CMAKE_SHARED_LIBRARY_SUFFIX}")

link_directories(${CMAKE_BINARY_DIR}/lib)
