project(tangram-core)

# Build core library dependencies.
add_subdirectory(deps)

add_library(tangram-core
  include/tangram/log.h
  include/tangram/map.h
  include/tangram/platform.h
  include/tangram/tangram.h
  include/tangram/data/clientGeoJsonSource.h
  include/tangram/data/properties.h
  include/tangram/data/propertyItem.h
  include/tangram/data/tileSource.h
  include/tangram/tile/tileID.h
  include/tangram/tile/tileTask.h
  include/tangram/util/types.h
  include/tangram/util/url.h
  include/tangram/util/variant.h
  src/map.cpp
  src/platform.cpp
  src/data/clientGeoJsonSource.cpp
  src/data/memoryCacheDataSource.h
  src/data/memoryCacheDataSource.cpp
  src/data/networkDataSource.h
  src/data/networkDataSource.cpp
  src/data/properties.cpp
  src/data/rasterSource.h
  src/data/rasterSource.cpp
  src/data/tileSource.cpp
  src/data/formats/geoJson.h
  src/data/formats/geoJson.cpp
  src/data/formats/mvt.h
  src/data/formats/mvt.cpp
  src/data/formats/topoJson.h
  src/data/formats/topoJson.cpp
  src/debug/frameInfo.h
  src/debug/frameInfo.cpp
  src/debug/textDisplay.h
  src/debug/textDisplay.cpp
  src/gl/framebuffer.h
  src/gl/framebuffer.cpp
  src/gl/glError.h
  src/gl/glError.cpp
  src/gl/glyphTexture.h
  src/gl/glyphTexture.cpp
  src/gl/hardware.h
  src/gl/hardware.cpp
  src/gl/mesh.h
  src/gl/mesh.cpp
  src/gl/primitives.h
  src/gl/primitives.cpp
  src/gl/renderState.h
  src/gl/renderState.cpp
  src/gl/shaderProgram.h
  src/gl/shaderProgram.cpp
  src/gl/shaderSource.h
  src/gl/shaderSource.cpp
  src/gl/texture.h
  src/gl/texture.cpp
  src/gl/vao.h
  src/gl/vao.cpp
  src/gl/vertexLayout.h
  src/gl/vertexLayout.cpp
  src/js/JavaScript.h
  src/js/JavaScriptFwd.h
  src/labels/curvedLabel.h
  src/labels/curvedLabel.cpp
  src/labels/label.h
  src/labels/label.cpp
  src/labels/labelCollider.h
  src/labels/labelCollider.cpp
  src/labels/labelProperty.h
  src/labels/labelProperty.cpp
  src/labels/labelSet.h
  src/labels/labelSet.cpp
  src/labels/labelManager.h
  src/labels/labelManager.cpp
  src/labels/spriteLabel.h
  src/labels/spriteLabel.cpp
  src/labels/textLabel.h
  src/labels/textLabel.cpp
  src/marker/marker.h
  src/marker/marker.cpp
  src/marker/markerManager.h
  src/marker/markerManager.cpp
  src/scene/ambientLight.h
  src/scene/ambientLight.cpp
  src/scene/dataLayer.h
  src/scene/dataLayer.cpp
  src/scene/directionalLight.h
  src/scene/directionalLight.cpp
  src/scene/drawRule.h
  src/scene/drawRule.cpp
  src/scene/filters.h
  src/scene/filters.cpp
  src/scene/importer.h
  src/scene/importer.cpp
  src/scene/light.h
  src/scene/light.cpp
  src/scene/pointLight.h
  src/scene/pointLight.cpp
  src/scene/scene.h
  src/scene/scene.cpp
  src/scene/sceneLayer.h
  src/scene/sceneLayer.cpp
  src/scene/sceneLoader.h
  src/scene/sceneLoader.cpp
  src/scene/spotLight.h
  src/scene/spotLight.cpp
  src/scene/spriteAtlas.h
  src/scene/spriteAtlas.cpp
  src/scene/stops.h
  src/scene/stops.cpp
  src/scene/styleContext.h
  src/scene/styleContext.cpp
  src/scene/styleMixer.h
  src/scene/styleMixer.cpp
  src/scene/styleParam.h
  src/scene/styleParam.cpp
  src/selection/featureSelection.h
  src/selection/featureSelection.cpp
  src/selection/selectionQuery.h
  src/selection/selectionQuery.cpp
  src/style/debugStyle.h
  src/style/debugStyle.cpp
  src/style/debugTextStyle.h
  src/style/debugTextStyle.cpp
  src/style/material.h
  src/style/material.cpp
  src/style/pointStyle.h
  src/style/pointStyle.cpp
  src/style/pointStyleBuilder.h
  src/style/pointStyleBuilder.cpp
  src/style/polygonStyle.h
  src/style/polygonStyle.cpp
  src/style/polylineStyle.h
  src/style/polylineStyle.cpp
  src/style/rasterStyle.h
  src/style/rasterStyle.cpp
  src/style/style.h
  src/style/style.cpp
  src/style/textStyle.h
  src/style/textStyle.cpp
  src/style/textStyleBuilder.h
  src/style/textStyleBuilder.cpp
  src/text/fontContext.h
  src/text/fontContext.cpp
  src/text/textUtil.h
  src/text/textUtil.cpp
  src/tile/tile.h
  src/tile/tile.cpp
  src/tile/tileBuilder.h
  src/tile/tileBuilder.cpp
  src/tile/tileManager.h
  src/tile/tileManager.cpp
  src/tile/tileTask.cpp
  src/tile/tileWorker.h
  src/tile/tileWorker.cpp
  src/util/builders.h
  src/util/builders.cpp
  src/util/dashArray.h
  src/util/dashArray.cpp
  src/util/extrude.h
  src/util/extrude.cpp
  src/util/floatFormatter.h
  src/util/floatFormatter.cpp
  src/util/geom.h
  src/util/geom.cpp
  src/util/inputHandler.h
  src/util/inputHandler.cpp
  src/util/jobQueue.h
  src/util/jobQueue.cpp
  src/util/json.h
  src/util/json.cpp
  src/util/mapProjection.h
  src/util/mapProjection.cpp
  src/util/rasterize.h
  src/util/rasterize.cpp
  src/util/stbImage.cpp
  src/util/url.cpp
  src/util/yamlPath.h
  src/util/yamlPath.cpp
  src/util/yamlUtil.h
  src/util/yamlUtil.cpp
  src/util/zipArchive.h
  src/util/zipArchive.cpp
  src/util/zlibHelper.h
  src/util/zlibHelper.cpp
  src/view/flyTo.h
  src/view/flyTo.cpp
  src/view/view.h
  src/view/view.cpp
  src/view/viewConstraint.h
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

# Link core library dependencies.
target_link_libraries(tangram-core
  PRIVATE
  css-color-parser-cpp
  yaml-cpp
  alfons
  double-conversion
  miniz
  z
)

# Add JavaScript implementation.
if(TANGRAM_JSCORE_ENABLED)
  target_sources(tangram-core PRIVATE src/js/JSCoreContext.cpp src/js/JSCoreContext.h)
  if (TANGRAM_USE_JSCORE_STATIC)
    target_link_libraries(tangram-core PRIVATE jscore-static)
  else()
    target_link_libraries(tangram-core PRIVATE "-framework JavaScriptCore")
  endif()
  target_compile_definitions(tangram-core PRIVATE TANGRAM_USE_JSCORE=1)
else()
  target_sources(tangram-core PRIVATE src/js/DuktapeContext.cpp src/js/DuktapeContext.h)
  target_link_libraries(tangram-core PRIVATE duktape)
endif()

# Add MBTiles implementation.
if(TANGRAM_MBTILES_DATASOURCE)
  target_sources(tangram-core PRIVATE src/data/mbtilesDataSource.cpp)
  target_link_libraries(tangram-core PRIVATE SQLiteCpp sqlite3)
  target_compile_definitions(tangram-core PRIVATE TANGRAM_MBTILES_DATASOURCE=1)
endif()

if(UNIX AND NOT APPLE)
  # SQLite needs dl dynamic library loader when Linux
  target_link_libraries(tangram-core PRIVATE dl)
endif()

if(TANGRAM_WARN_ON_RULE_CONFLICT)
  target_compile_definitions(tangram-core
    PRIVATE
    TANGRAM_WARN_ON_RULE_CONFLICT)
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

if(CMAKE_VERSION VERSION_GREATER "3.8")
  # Create source groups for Xcode - this source_group syntax requires version >=3.8
  get_target_property(TANGRAM_CORE_SOURCES tangram-core SOURCES)
  source_group(TREE ${PROJECT_SOURCE_DIR} FILES ${TANGRAM_CORE_SOURCES})
endif()
