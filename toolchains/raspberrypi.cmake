# options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fpermissive -g")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_CXX_FLAGS} -L/opt/vc/lib/ -lGLESv2 -lEGL -lbcm_host -lvchiq_arm -lvcos -lrt -lpthread")
set(CXX_FLAGS_DEBUG "-g -O0")
set(EXECUTABLE_NAME "tangram")

add_definitions(-DPLATFORM_RPI)

# check for c++11 compiler
execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
	if(NOT (GCC_VERSION VERSION_GREATER 4.8 OR GCC_VERSION VERSION_EQUAL 4.8))
		message(FATAL_ERROR "Please install g++ version 4.8 or greater")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
	endif()
else()
	message(FATAL_ERROR "Please install a C++11 compatible compiler")
endif()

# add sources and include headers
find_sources_and_include_directories(
	${PROJECT_SOURCE_DIR}/rpi/src/*.h 
	${PROJECT_SOURCE_DIR}/rpi/src/*.cpp)

# add sources and include headers
find_sources_and_include_directories(
    ${PROJECT_SOURCE_DIR}/linux/src/urlWorker.*
    ${PROJECT_SOURCE_DIR}/linux/src/urlWorker.*)

# include headers for rpi-installed libraries
include_directories(/opt/vc/include/)
include_directories(/opt/vc/include/interface/vcos/pthreads)
include_directories(/opt/vc/include/interface/vmcs_host/linux)

# load core library
add_subdirectory(${PROJECT_SOURCE_DIR}/core)
include_directories(${CORE_INCLUDE_DIRS})
include_directories(${CORE_LIBRARIES_INCLUDE_DIRS})

# link and build functions
function(link_libraries)

	target_link_libraries(${EXECUTABLE_NAME} -lcurl) #use system libcurl
    target_link_libraries(${EXECUTABLE_NAME} core)

endfunction()

function(build) 

    add_executable(${EXECUTABLE_NAME} ${SOURCES})

    file(GLOB_RECURSE RESOURCES ${PROJECT_SOURCE_DIR}/core/resources/*)
    foreach(_resource ${RESOURCES})
        file(COPY ${_resource} DESTINATION ${PROJECT_SOURCE_DIR}/build/rpi/bin)
    endforeach()

endfunction()
