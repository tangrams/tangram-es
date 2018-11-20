# Build Google Benchmark library.
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "")
set(BENCHMARK_ENABLE_LTO OFF CACHE BOOL "")
add_subdirectory(benchmark)
target_compile_options(benchmark PRIVATE -O3 -DNDEBUG)

set(BENCH_SOURCES
  src/benchGeometryBuilder.cpp
  src/benchStyleContext.cpp
  src/benchTileBuilder.cpp
  src/benchTileSource.cpp
  src/template.cpp
)

# create an executable per bench
foreach(_src_file_path ${BENCH_SOURCES})
  string(REPLACE ".cpp" "" bench ${_src_file_path})
  string(REGEX MATCH "([^/]*)$" bench_name ${bench})

  set(EXECUTABLE_NAME "${bench_name}.out")

  add_executable(${EXECUTABLE_NAME} ${_src_file_path})

  target_include_directories(${EXECUTABLE_NAME}
    PRIVATE
    benchmark/include
  )

  # Use all include directories from tangram-core because tests interact with internal classes.
  target_include_directories(${EXECUTABLE_NAME} PRIVATE
    $<TARGET_PROPERTY:tangram-core,INCLUDE_DIRECTORIES>
  )

  target_link_libraries(${EXECUTABLE_NAME}
    tangram-core
    benchmark
    platform_mock
    -lpthread
  )

  set_target_properties(${EXECUTABLE_NAME}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bench"
  )

  add_resources(${EXECUTABLE_NAME} "${PROJECT_SOURCE_DIR}/scenes" "res")

endforeach()

