
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
set(WRAPPER_LIB "tangram")

# check if the includes files changed...
set(SWIG_MODULE_${WRAPPER_LIB}_EXTRA_DEPS tangram_api.i basic_vector.i)

swig_add_module(${WRAPPER_LIB} python tangram.i ${SOURCES})

if(${PLATFORM_TARGET} MATCHES "linux")
  swig_link_libraries(${WRAPPER_LIB}
    LINK_PUBLIC
    # ${CORE_LIBRARY}
    tangram_shared
    LINK_PRIVATE
    ${PYTHON_LIBRARIES}
    ${OPENGL_LIBRARIES}
    curl
    # FIXME should not be required
    glfw
    -lpthread)
endif()


set(TARGET_NAME ${SWIG_MODULE_${WRAPPER_LIB}_REAL_NAME})

# http://stackoverflow.com/questions/13920072/cmake-always-run-command-regardless-of-any-dependency

# Custom target will always cause its dependencies to be evaluated and is
# run by default
add_custom_target(dummy_target ALL DEPENDS custom_output )

# custom_output will always be rebuilt because it depends on always_rebuild
# add_custom_command(
#     OUTPUT custom_output
#     COMMAND command_that_produces_custom_output
#     DEPENDS always_rebuild)
# Dummy output which is never actually produced. Anything that depends on
# this will always be rebuilt.
add_custom_command(OUTPUT always_rebuild COMMAND cmake -E echo)

add_custom_command(
  # TARGET dummy_target # ${TARGET_NAME}
  #POST_BUILD
  OUTPUT custom_output
  DEPENDS always_rebuild
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_CURRENT_SOURCE_DIR}/examples
  ${CMAKE_BINARY_DIR}/bin)
add_custom_command(TARGET ${TARGET_NAME}
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy
  ${CMAKE_BINARY_DIR}/swig/tangram.py
  ${CMAKE_BINARY_DIR}/bin)
