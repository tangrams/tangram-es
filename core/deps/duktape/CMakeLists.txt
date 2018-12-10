add_library(duktape duktape.c extstr.c)

target_compile_options(duktape PRIVATE
  -fstrict-aliasing
  -fomit-frame-pointer
  -std=c99
  -Wall)

target_include_directories(duktape
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR})
