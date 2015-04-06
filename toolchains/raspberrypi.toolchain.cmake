set(CMAKE_CXX_COMPILER "g++")
execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)

if(NOT GCC_VERSION VERSION_GREATER 4.7 OR GCC_VERSION VERSION_EQUAL 4.7)
	set(GNU_CXX_COMPILER "/usr/bin/g++-4.7")
	if(EXISTS ${GNU_CXX_COMPILER}) 
		set(CMAKE_CXX_COMPILER ${GNU_CXX_COMPILER})
	else()
		message(FATAL_ERROR "Please install g++ version 4.7 or greater")
	endif()
endif()

