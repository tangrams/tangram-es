# options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11 -fpermissive")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_CXX_FLAGS} -L/opt/vc/lib/ -lGLESv2 -lEGL -lbcm_host -lvchiq_arm -lvcos -lrt")
set(CXX_FLAGS_DEBUG "-g -O0")
set(EXECUTABLE_NAME "tangram")

add_definitions(-DPLATFORM_RPI)

# include headers for rpi-installed libraries
include_directories(/opt/vc/include/)
include_directories(/opt/vc/include/interface/vcos/pthreads)
include_directories(/opt/vc/include/interface/vmcs_host/linux)

# load core library
include_directories(${PROJECT_SOURCE_DIR}/core/include/)
include_directories(${PROJECT_SOURCE_DIR}/core/include/jsoncpp/)
add_subdirectory(${PROJECT_SOURCE_DIR}/core)
include_recursive_dirs(${PROJECT_SOURCE_DIR}/core/src/*.h)

# add sources and include headers
set(OSX_EXTENSIONS_FILES *.mm *.cpp)
foreach(_ext ${OSX_EXTENSIONS_FILES})
    find_sources_and_include_directories(
        ${PROJECT_SOURCE_DIR}/rpi/src/*.h 
        ${PROJECT_SOURCE_DIR}/rpi/src/${_ext})
endforeach()

# link and build functions
function(link_libraries)

	file(GLOB_RECURSE OBJ_FILES ${PROJECT_SOURCE_DIR}/rpi/precompiled/libtess2/*.o)
	target_link_libraries(${EXECUTABLE_NAME} -lcurl) #use system libcurl
    target_link_libraries(${EXECUTABLE_NAME} ${OBJ_FILES})
	#target_link_libraries(${EXECUTABLE_NAME} ${PROJECT_SOURCE_DIR}/rpi/precompiled/libtess2/libtess2.a)
    target_link_libraries(${EXECUTABLE_NAME} core)

endfunction()

function(build) 

    add_executable(${EXECUTABLE_NAME} ${SOURCES})

endfunction()
