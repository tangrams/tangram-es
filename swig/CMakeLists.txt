
find_package(SWIG REQUIRED)
find_package(OpenGL REQUIRED)
#find_package(PythonLibs 2.7 EXACT REQUIRED)
find_package(PythonLibs REQUIRED)


include(${SWIG_USE_FILE})
include_directories(${PYTHON_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/core/src)
include_directories(${PROJECT_SOURCE_DIR}/linux/src)
include_directories(${PROJECT_SOURCE_DIR}/core/common)

set(SOURCES
  ${PROJECT_SOURCE_DIR}/linux/src/urlWorker.cpp
  ${PROJECT_SOURCE_DIR}/linux/src/platform_linux.cpp
  ${PROJECT_SOURCE_DIR}/core/common/platform_gl.cpp)

set(CMAKE_SWIG_FLAGS "")
set_source_files_properties(tangram.i PROPERTIES CPLUSPLUS ON)

set(CMAKE_CXX_FLAGS "-g ${CMAKE_CXX_FLAGS}")

set(TANGRAM_BOLTS "tangram")

# check if the includes files changed...
set(SWIG_MODULE_${TANGRAM_BOLTS}_EXTRA_DEPS basic_vector.i)

swig_add_module(${TANGRAM_BOLTS} python tangram.i ${SOURCES})

if(${PLATFORM_TARGET} MATCHES "linux")
  swig_link_libraries(${TANGRAM_BOLTS}
    LINK_PUBLIC
    # ${CORE_LIBRARY}
    tangram_shared
    LINK_PRIVATE
    ${PYTHON_LIBRARIES}
    ${OPENGL_LIBRARIES}
    curl glfw
    -lpthread)
endif()

set_source_files_properties(tangram_api.i PROPERTIES CPLUSPLUS ON)

set(TANGRAM_API "tangram_api")

swig_add_module(${TANGRAM_API} python tangram_api.i ${SOURCES})

if(${PLATFORM_TARGET} MATCHES "linux")
  swig_link_libraries(${TANGRAM_API}
    LINK_PUBLIC
    # ${CORE_LIBRARY}
    tangram_shared
    LINK_PRIVATE
    ${PYTHON_LIBRARIES}
    ${OPENGL_LIBRARIES}
    curl glfw
    -lpthread)
endif()

set(TARGET_BOLTS ${SWIG_MODULE_${TANGRAM_BOLTS}_REAL_NAME})
set(TARGET_API ${SWIG_MODULE_${TANGRAM_API}_REAL_NAME})

add_custom_command(TARGET ${TARGET_BOLTS}
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy
  ${CMAKE_BINARY_DIR}/swig/tangram.py
  ${CMAKE_BINARY_DIR}/bin)
add_custom_command(TARGET ${TARGET_API}
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy
  ${CMAKE_BINARY_DIR}/swig/tangram_api.py
  ${CMAKE_BINARY_DIR}/bin)

# http://stackoverflow.com/questions/13920072/cmake-always-run-command
# -regardless-of-any-dependency
# Custom target will always cause its dependencies to be evaluated and is
# run by default
add_custom_target(dummy_target ALL DEPENDS custom_output )

# Dummy output which is never actually produced. Anything that depends on
# this will always be rebuilt.
add_custom_command(OUTPUT always_rebuild COMMAND cmake -E echo)

add_custom_command(
  OUTPUT custom_output
  DEPENDS always_rebuild
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_CURRENT_SOURCE_DIR}/examples
  ${CMAKE_BINARY_DIR}/bin)
