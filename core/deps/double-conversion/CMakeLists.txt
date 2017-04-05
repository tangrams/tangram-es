project(double-conversion)

add_library(double-conversion
  src/bignum-dtoa.cc
  src/bignum.cc
  src/cached-powers.cc
  src/diy-fp.cc
  src/double-conversion.cc
  src/fast-dtoa.cc
  src/fixed-dtoa.cc
  src/strtod.cc)

target_include_directories(double-conversion
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src)
