add_executable(tangram-demo
  ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

add_dependencies(tangram-demo tangram-core)
target_include_directories(tangram-demo
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
)
list(APPEND TANGRAM_DEMO_LIBS glfw)
list(APPEND TANGRAM_DEMO_LIBS tangram-core)
target_link_libraries(tangram-demo ${TANGRAM_DEMO_LIBS})

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
    set_target_properties(tangram-demo PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/bin)
  endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )
endif()
add_resources(tangram-demo "${CMAKE_CURRENT_SOURCE_DIR}/../../scenes")
