
if (USE_EXTERNAL_LIBS)
  include(${EXTERNAL_LIBS_DIR}/benchmark.cmake)
endif()

file(GLOB BENCH_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

# create an executable per test
foreach(_src_file_path ${BENCH_SOURCES})
  string(REPLACE ".cpp" "" bench ${_src_file_path})
  string(REGEX MATCH "([^/]*)$" bench_name ${bench})

  set(EXECUTABLE_NAME "${bench_name}.out")

  add_executable(${EXECUTABLE_NAME} ${_src_file_path})

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

