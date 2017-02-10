# Build Google Benchmark library.
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "")
set(BENCHMARK_ENABLE_LTO OFF CACHE BOOL "")
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/benchmark)
target_compile_options(benchmark PRIVATE -O3 -DNDEBUG)

file(GLOB BENCH_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

# create an executable per bench
foreach(_src_file_path ${BENCH_SOURCES})
    string(REPLACE ".cpp" "" bench ${_src_file_path})
    string(REGEX MATCH "([^/]*)$" bench_name ${bench})

    set(EXECUTABLE_NAME "${bench_name}.out")

    add_executable(${EXECUTABLE_NAME} ${_src_file_path})

    target_include_directories(${EXECUTABLE_NAME}
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/benchmark/include)

    target_link_libraries(${EXECUTABLE_NAME}
        ${CORE_LIBRARY}
        benchmark
        platform_mock
        -lpthread)

    set_target_properties(${EXECUTABLE_NAME}
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/bench")

    add_resources(${EXECUTABLE_NAME} "${PROJECT_SOURCE_DIR}/scenes")

endforeach()

