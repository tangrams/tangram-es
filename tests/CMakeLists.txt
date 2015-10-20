
file(GLOB TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/unit/*.cpp)

# create an executable per test
foreach(_src_file_path ${TEST_SOURCES})
  string(REPLACE ".cpp" "" test_case ${_src_file_path})
  string(REGEX MATCH "([^/]*)$" test_name ${test_case})

  set(EXECUTABLE_NAME "${test_name}.out")
  add_executable(${EXECUTABLE_NAME} ${_src_file_path})

  target_link_libraries(${EXECUTABLE_NAME}
    ${CORE_LIBRARY}
    platform_mock -lpthread)

  target_include_directories(${EXECUTABLE_NAME}
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/catch)

  set_target_properties(${EXECUTABLE_NAME}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/unit"
    )

  add_dependencies(${EXECUTABLE_NAME} copy_resources)

endforeach()
