
# benchmark #
#############
option(BENCHMARK_ENABLE_TESTING OFF)
option(BENCHMARK_ENABLE_LTO OFF)
set(CMAKE_BUILD_TYPE Release)
add_subdirectory(benchmark)

file(GLOB BENCH_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

# create an executable per test
foreach(_src_file_path ${BENCH_SOURCES})
  string(REPLACE ".cpp" "" bench ${_src_file_path})
  string(REGEX MATCH "([^/]*)$" bench_name ${bench})

  set(EXECUTABLE_NAME "${bench_name}.out")

  add_executable(${EXECUTABLE_NAME} ${_src_file_path})

  target_link_libraries(${EXECUTABLE_NAME}
    benchmark core platform_mock -lpthread)

  target_include_directories(${EXECUTABLE_NAME}
    PUBLIC
    ${PROJECT_SOURCE_DIR}/core/dependencies/benchmark/include)

  set_target_properties(${EXECUTABLE_NAME}
    PROPERTIES
    #ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    #LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/bench"
    )

  add_dependencies(${EXECUTABLE_NAME} copy_resources)

endforeach()

