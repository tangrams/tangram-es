
add_library(platform_test
  ${CMAKE_CURRENT_SOURCE_DIR}/src/catch.cpp
)

target_link_libraries(platform_test
  PUBLIC
  platform_mock
  -lpthread
)

target_include_directories(platform_test
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/catch
)

set(TEST_SOURCES
  unit/curlTests.cpp
  unit/drawRuleTests.cpp
  unit/dukTests.cpp
  unit/fileTests.cpp
  unit/flyToTest.cpp
  unit/jobQueueTests.cpp
  unit/labelsTests.cpp
  unit/labelTests.cpp
  unit/layerTests.cpp
  unit/lineWrapTests.cpp
  unit/lngLatTests.cpp
  unit/mapProjectionTests.cpp
  unit/meshTests.cpp
  unit/sceneImportTests.cpp
  unit/sceneLoaderTests.cpp
  unit/sceneUpdateTests.cpp
  unit/stopsTests.cpp
  unit/styleMixerTests.cpp
  unit/styleSortingTests.cpp
  unit/styleUniformsTests.cpp
  unit/textureTests.cpp
  unit/tileIDTests.cpp
  unit/tileManagerTests.cpp
  unit/urlTests.cpp
  unit/yamlFilterTests.cpp
  unit/yamlUtilTests.cpp
)

# create an executable per test
foreach(_src_file_path ${TEST_SOURCES})
  string(REPLACE ".cpp" "" test_case ${_src_file_path})
  string(REGEX MATCH "([^/]*)$" test_name ${test_case})

  set(EXECUTABLE_NAME "${test_name}.out")
  add_executable(${EXECUTABLE_NAME} ${_src_file_path})

  target_link_libraries(${EXECUTABLE_NAME}
    tangram-core
    platform_test
  )

  # Use all include directories from tangram-core because tests interact with internal classes.
  target_include_directories(${EXECUTABLE_NAME} PRIVATE
    $<TARGET_PROPERTY:tangram-core,INCLUDE_DIRECTORIES>
  )

  set_target_properties(${EXECUTABLE_NAME}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/unit"
  )

endforeach()

# Copy resources into output directory (only needs to be performed for one target)
add_resources(${EXECUTABLE_NAME} "${PROJECT_SOURCE_DIR}/scenes")
