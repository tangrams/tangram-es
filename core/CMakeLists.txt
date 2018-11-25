project(tangram-core)

# Build core library dependencies.
add_subdirectory(deps)

add_library(tangram-core
  src/map.cpp
  src/platform.cpp
  src/data/clientGeoJsonSource.cpp
  src/data/mbtilesDataSource.cpp
  src/data/memoryCacheDataSource.cpp
  src/data/networkDataSource.cpp
  src/data/properties.cpp
  src/data/rasterSource.cpp
  src/data/tileSource.cpp
  src/data/formats/geoJson.cpp
  src/data/formats/mvt.cpp
  src/data/formats/topoJson.cpp
  src/debug/frameInfo.cpp
  src/debug/textDisplay.cpp
  src/gl/framebuffer.cpp
  src/gl/glError.cpp
  src/gl/hardware.cpp
  src/gl/mesh.cpp
  src/gl/primitives.cpp
  src/gl/renderState.cpp
  src/gl/shaderProgram.cpp
  src/gl/shaderSource.cpp
  src/gl/texture.cpp
  src/gl/vao.cpp
  src/gl/vertexLayout.cpp
  src/labels/curvedLabel.cpp
  src/labels/label.cpp
  src/labels/labelCollider.cpp
  src/labels/labelProperty.cpp
  src/labels/labelSet.cpp
  src/labels/labels.cpp
  src/labels/spriteLabel.cpp
  src/labels/textLabel.cpp
  src/marker/marker.cpp
  src/marker/markerManager.cpp
  src/scene/ambientLight.cpp
  src/scene/dataLayer.cpp
  src/scene/directionalLight.cpp
  src/scene/drawRule.cpp
  src/scene/filters.cpp
  src/scene/importer.cpp
  src/scene/light.cpp
  src/scene/pointLight.cpp
  src/scene/scene.cpp
  src/scene/sceneLayer.cpp
  src/scene/sceneLoader.cpp
  src/scene/spotLight.cpp
  src/scene/spriteAtlas.cpp
  src/scene/stops.cpp
  src/scene/styleContext.cpp
  src/scene/styleMixer.cpp
  src/scene/styleParam.cpp
  src/selection/featureSelection.cpp
  src/selection/selectionQuery.cpp
  src/style/debugStyle.cpp
  src/style/debugTextStyle.cpp
  src/style/material.cpp
  src/style/pointStyle.cpp
  src/style/pointStyleBuilder.cpp
  src/style/polygonStyle.cpp
  src/style/polylineStyle.cpp
  src/style/rasterStyle.cpp
  src/style/style.cpp
  src/style/textStyle.cpp
  src/style/textStyleBuilder.cpp
  src/text/fontContext.cpp
  src/text/textUtil.cpp
  src/tile/tile.cpp
  src/tile/tileBuilder.cpp
  src/tile/tileManager.cpp
  src/tile/tileTask.cpp
  src/tile/tileWorker.cpp
  src/util/builders.cpp
  src/util/dashArray.cpp
  src/util/extrude.cpp
  src/util/floatFormatter.cpp
  src/util/geom.cpp
  src/util/inputHandler.cpp
  src/util/jobQueue.cpp
  src/util/json.cpp
  src/util/mapProjection.cpp
  src/util/rasterize.cpp
  src/util/url.cpp
  src/util/yamlPath.cpp
  src/util/yamlUtil.cpp
  src/util/zipArchive.cpp
  src/util/zlibHelper.cpp
  src/view/flyTo.cpp
  src/view/view.cpp
  src/view/viewConstraint.cpp
)

# Include headers from core library and dependencies.
target_include_directories(tangram-core
  PUBLIC
  include/tangram
  # TODO: These headers shouldn't be public, but Platform classes use them.
  src
  # TODO: This header shouldn't be public, but we use it in propertyItem.h
  deps/glm
  deps/variant/include
  PRIVATE
  generated
  deps
  deps/earcut/include
  deps/isect2d/include
  deps/hash-library
  deps/mapbox
  deps/pbf
  deps/rapidjson
  deps/sdf
  deps/SQLiteCpp
  deps/stb
  deps/miniz
  deps/geojson-vt-cpp/geometry.hpp/include
  deps/geojson-vt-cpp/geojson.hpp/include
  deps/geojson-vt-cpp/include
  deps/double-conversion/include
)

# Choose JavaScript implementation.
if(TANGRAM_USE_JAVASCRIPTCORE)
  target_sources(tangram-core PRIVATE src/js/JSCoreContext.cpp)
  if (TANGRAM_USE_JAVASCRIPTCORE_STATIC)
    find_package(ICU COMPONENTS io i18n REQUIRED)
    link_directories(${ICU_LIBRARY_DIRS})
    target_link_libraries(tangram-core
      PRIVATE
      ${ICU_LIBRARIES}
      "${PROJECT_SOURCE_DIR}/deps/jscore/lib/libJavaScriptCore.a"
      "${PROJECT_SOURCE_DIR}/deps/jscore/lib/libWTF.a"
      "${PROJECT_SOURCE_DIR}/deps/jscore/lib/libbmalloc.a")
    target_include_directories(tangram-core
      PRIVATE
      "${PROJECT_SOURCE_DIR}/deps/jscore/include")
  else()
    target_link_libraries(tangram-core PRIVATE "-framework JavaScriptCore")
  endif()
endif()

if(TANGRAM_USE_DUKTAPE)
  target_sources(tangram-core PRIVATE src/js/DuktapeContext.cpp)
  target_link_libraries(tangram-core PRIVATE duktape)
endif()

# Link core library dependencies.
target_link_libraries(tangram-core
  PRIVATE
  css-color-parser-cpp
  yaml-cpp
  alfons
  SQLiteCpp
  sqlite3
  double-conversion
  miniz
  z
)


if(UNIX AND NOT APPLE)
  # SQLite needs dl dynamic library loader when Linux
  target_link_libraries(tangram-core PRIVATE dl)
endif()

if(TANGRAM_WARN_ON_RULE_CONFLICT)
  add_definitions(-DTANGRAM_WARN_ON_RULE_CONFLICT)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  # using regular Clang or AppleClang
  target_compile_options(tangram-core
    PUBLIC
    -Wno-gnu-anonymous-struct
    -Wno-nested-anon-types
    -Wno-gnu-zero-variadic-macro-arguments
  )
endif()

# C++14 is required for compiling tangram-core and for using the interface headers.
# Making this option PUBLIC declares it as a requirement for targets linking against tangram-core.
target_compile_options(tangram-core
  PUBLIC -std=c++14
)

# make groups for xcode
group_recursive_sources(src "src")

# We include GLSL shader sources into the library by generating header files with the source text
# printed into a string constant. A CMake script generates one of these headers for each shader source
# file and they each become a custom target that the core library depends on.
set(SHADER_FILES
  shaders/ambientLight.glsl
  shaders/debug.fs
  shaders/debug.vs
  shaders/debugPrimitive.fs
  shaders/debugPrimitive.vs
  shaders/debugTexture.fs
  shaders/debugTexture.vs
  shaders/directionalLight.glsl
  shaders/lights.glsl
  shaders/material.glsl
  shaders/point.fs
  shaders/point.vs
  shaders/pointLight.glsl
  shaders/polygon.fs
  shaders/polygon.vs
  shaders/polyline.fs
  shaders/polyline.vs
  shaders/rasters.glsl
  shaders/sdf.fs
  shaders/selection.fs
  shaders/spotLight.glsl
  shaders/text.fs
  shaders/text.vs
)
foreach(_shader ${SHADER_FILES})
  get_filename_component(_shader_name "${_shader}" NAME_WE)
  get_filename_component(_shader_ext "${_shader}" EXT)
  string(REGEX REPLACE "\\." "" _shader_ext "${_shader_ext}")
  set(_shader_name "${_shader_name}_${_shader_ext}")
  set(SHADER_OUTPUT "${PROJECT_SOURCE_DIR}/generated/${_shader_name}.h")
  add_custom_command(
    OUTPUT "${SHADER_OUTPUT}"
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    MAIN_DEPENDENCY "${_shader}"
    COMMAND ${CMAKE_COMMAND} -Din="${_shader}" -Dout="${SHADER_OUTPUT}" -Dname="${_shader_name}" -P ${CMAKE_CURRENT_SOURCE_DIR}/shaders.cmake
  )
  list(APPEND SHADER_OUTPUT_FILES "${SHADER_OUTPUT}")
endforeach()
add_custom_target(shader-headers DEPENDS ${SHADER_OUTPUT_FILES})
add_dependencies(tangram-core shader-headers)
