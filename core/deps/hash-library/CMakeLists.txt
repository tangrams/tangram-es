project(hashlib)

add_library(hashlib md5.cpp)

target_include_directories(hashlib PUBLIC 
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:include>"
)

tangram_install(hashlib)
tangram_install_files("${CMAKE_CURRENT_SOURCE_DIR}/md5.h")
