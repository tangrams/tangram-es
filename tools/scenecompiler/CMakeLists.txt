if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/static)
  set(GLSLOPT_LIBRARIES
    ${CMAKE_CURRENT_SOURCE_DIR}/static/lib/libglsl_optimizer.a
    ${CMAKE_CURRENT_SOURCE_DIR}/static/lib/libmesa.a
    ${CMAKE_CURRENT_SOURCE_DIR}/static/lib/libglcpp-library.a)

  set(GLSLOPT_INCLUDE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/static/include)

elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/glsl-optimizer)
  add_subdirectory(glsl-optimizer)
  set(GLSLOPT_LIBRARIES
    glsl_optimizer)

elseif(TANGRAM_BUILD_GLSL_OPTIMIZER_AS_EXTERNAL_CMAKE_PROJECT)
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

  set(GLSLOPT_INCLUDE_DIR
    ${CORE_INCLUDE_DIRS}
    ${CORE_LIBRARIES_INCLUDE_DIRS}
    ${GLSLOPT_INCLUDE_DIR})

  set(GLSLOPT_LIBRARIES
    ${binary_dir}/libglsl_optimizer.a
    ${binary_dir}/libmesa.a
    ${binary_dir}/libglcpp-library.a)

else()
  message(STATUS "In tool/scenecompiler directory run:")
  message(STATUS "git clone --depth=1 git://github.com/hjanetzek/glsl-optimizer.git")
  message(STATUS "Or use -DTANGRAM_BUILD_GLSL_OPTIMIZER_AS_EXTERNAL_CMAKE_PROJECT=1")
endif()

add_executable(scenecompiler
  main.cpp
  ${CMAKE_SOURCE_DIR}/tests/src/mockPlatform.cpp
  ${CMAKE_SOURCE_DIR}/tests/src/gl_mock.cpp)

target_include_directories( scenecompiler
  PRIVATE
  ${GLSLOPT_INCLUDE_DIR}
  ${CMAKE_SOURCE_DIR}/core/deps
  ${CMAKE_SOURCE_DIR}/core/deps/isect2d/include
  ${CMAKE_SOURCE_DIR}/core/deps/variant/include
  ${CMAKE_SOURCE_DIR}/core/deps/mapbox
  ${CMAKE_SOURCE_DIR}/tests/src
  )

target_link_libraries(scenecompiler
  tangram-core
  platform_mock
  yaml-cpp
  glfw
  ${GLFW_LIBRARIES}
  ${GLSLOPT_LIBRARIES})
