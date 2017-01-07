enable_testing()

add_library(platform_test
  ${CMAKE_CURRENT_SOURCE_DIR}/src/catch.cpp)

set (TANGRAM_TEST_LINK_LIBRARIES platform_mock)
if (NOT WIN32)
  list (APPEND TANGRAM_TEST_LINK_LIBRARIES -lpthread)
endif()
target_link_libraries(platform_test
  PUBLIC
  ${TANGRAM_TEST_LINK_LIBRARIES}
)

target_include_directories(platform_test
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/catch)

file(GLOB TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/unit/*.cpp)

# create an executable per test
foreach(_src_file_path ${TEST_SOURCES})
  string(REPLACE ".cpp" "" test_case ${_src_file_path})
  string(REGEX MATCH "([^/]*)$" test_name ${test_case})

  set(EXECUTABLE_NAME "${test_name}.out")
  add_executable(${EXECUTABLE_NAME} ${_src_file_path})

  target_link_libraries(${EXECUTABLE_NAME}
    ${CORE_LIBRARY}
    platform_test)

  set_target_properties(${EXECUTABLE_NAME}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/unit"
  )

  add_test(
    NAME ${EXECUTABLE_NAME}
    COMMAND $<TARGET_FILE:${EXECUTABLE_NAME}>
    WORKING_DIRECTORY $<TARGET_FILE_DIR:${EXECUTABLE_NAME}>
  )

endforeach()

# Copy resources into output directory (only needs to be performed for one target)
add_resources(${EXECUTABLE_NAME} "${PROJECT_SOURCE_DIR}/scenes")
