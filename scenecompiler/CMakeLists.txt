if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/glsl-optimizer)

add_subdirectory(glsl-optimizer)

include_directories(
  ${CORE_INCLUDE_DIRS}
  ${CORE_LIBRARIES_INCLUDE_DIRS}
  ${GLSLOPT_INCLUDE_DIR}
)

add_executable( scenecompiler
  main.cpp
  platform_mock.cpp
)

target_link_libraries( scenecompiler
  core
  glfw
  ${GLFW_LIBRARIES}
  glsl_optimizer
)

elseif() # (NOT EXISTS "glsl-optimizer")

include(ExternalProject)

ExternalProject_Add(glsl-optimizer
  GIT_REPOSITORY git://github.com/hjanetzek/glsl-optimizer.git
  #GIT_REPOSITORY /home/jeff/work/glsl-optimizer/.git
  GIT_TAG c0050010d3f19111a54afa991b3adbdb0770e89e
  TIMEOUT 10
  #CONFIGURE_COMMAND ""
  INSTALL_COMMAND ""
  #UPDATE_COMMAND ""
  #BUILD_COMMAND ""
)

ExternalProject_Get_Property(glsl-optimizer source_dir)
ExternalProject_Get_Property(glsl-optimizer binary_dir)
ExternalProject_Get_Property(glsl-optimizer install_dir)

set(GLSLOPT_INCLUDE_DIR "${source_dir}/src/glsl" CACHE INTERNAL "glsl-optimizer")
set(GLSLOPT_LIB_DIR "${binary_dir}" CACHE INTERNAL "glsl-optimizer")
set(GLSLOPT_INSTALL_DIR "${install_dir}" CACHE INTERNAL "glsl-optimizer")

include_directories(
  ${CORE_INCLUDE_DIRS}
  ${CORE_LIBRARIES_INCLUDE_DIRS}
  ${GLSLOPT_INCLUDE_DIR}
)

add_executable( scenecompiler
  main.cpp
  platform_mock.cpp
)

target_link_libraries( scenecompiler
  core
  glfw
  ${GLFW_LIBRARIES}
  #glsl_optimizer
  # ${GLSLOPT_LIB_DIR}/libglsl_optimizer.a
  # ${GLSLOPT_LIB_DIR}/libmesa.a
  # ${GLSLOPT_LIB_DIR}/libglcpp-library.a
  ${binary_dir}/libglsl_optimizer.a
  ${binary_dir}/libmesa.a
  ${binary_dir}/libglcpp-library.a
)

# target_include_directories( scenecompiler
#   ${CORE_LIBRARIES_INCLUDE_DIRS}
#   ${GLSLOPT_INCLUDE_DIR}
# )
else()
  message(STATUS "In scenecompiler directory run: git clone --depth=1 git://github.com/hjanetzek/glsl-optimizer.git")
endif()
