# options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fpermissive -g")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_CXX_FLAGS} -L/opt/vc/lib/ -lGLESv2 -lEGL -lbcm_host -lvchiq_arm -lvcos -lrt -lpthread")
set(CXX_FLAGS_DEBUG "-g -O0")
set(EXECUTABLE_NAME "tangram")

add_definitions(-DPLATFORM_RPI)

check_unsupported_compiler_version()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

# add sources and include headers
find_sources_and_include_directories(
  ${PROJECT_SOURCE_DIR}/platforms/rpi/src/*.h
  ${PROJECT_SOURCE_DIR}/platforms/rpi/src/*.cpp)

set(COMMON_SOURCES
    ${PROJECT_SOURCE_DIR}/platforms/common/urlClient.cpp
    ${PROJECT_SOURCE_DIR}/platforms/common/platform_gl.cpp)
include_directories(${PROJECT_SOURCE_DIR}/platforms/common)

# add linux dependencies
set(LINUX_SOURCES
    ${PROJECT_SOURCE_DIR}/platforms/linux/src/platform_linux.cpp)
include_directories(${PROJECT_SOURCE_DIR}/platforms/linux/src)

# include headers for rpi-installed libraries
include_directories(/opt/vc/include/)
include_directories(/opt/vc/include/interface/vcos/pthreads)
include_directories(/opt/vc/include/interface/vmcs_host/linux)

# load core library
add_subdirectory(${PROJECT_SOURCE_DIR}/core)

add_executable(${EXECUTABLE_NAME} ${SOURCES} ${COMMON_SOURCES} ${LINUX_SOURCES})

target_link_libraries(${EXECUTABLE_NAME}
  ${CORE_LIBRARY} -lcurl)

add_resources(${EXECUTABLE_NAME} "${PROJECT_SOURCE_DIR}/scenes")
