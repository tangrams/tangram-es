project(hashlib)

add_library(hashlib md5.cpp)

target_include_directories(hashlib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
