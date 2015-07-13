#pragma once

#include "data/tileData.h"
#include "material.h"
#include "gl.h"
#include "scene/sceneLayer.h"
#include "styleParamMap.h"
#include "util/shaderProgram.h"

#include "csscolorparser.hpp"

#include <bitset>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

class Light;
class MapTile;
class MapProjection;
class VboMesh;
class VertexLayout;
class View;
class Scene;

enum class LightingType : char {
    none,
    vertex,
    fragment
};

namespace Tangram {
    struct Value;
    using Context = std::unordered_map<std::string, Value*>;
}

/* Means of constructing and rendering map geometry
 *
 * A Style defines a way to
 *   1. Construct map geometry into a mesh for drawing and
 *   2. Render the resulting mesh in a scene
 * Style implementations must provide functions to construct
 * a <VertexLayout> for their geometry, construct a <ShaderProgram>
 * for rendering meshes, and build point, line, and polygon
 * geometry into meshes. See <PolygonStyle> for a basic implementation.
 */
class Style {

protected:

    /* The platform pixel scale */
    float m_pixelScale = 1.0;

    /* Unique name for a style instance */
    std::string m_name;

    /* <ShaderProgram> used to draw meshes using this style */
    std::shared_ptr<ShaderProgram> m_shaderProgram = std::make_shared<ShaderProgram>();

    /* <VertexLayout> shared between meshes using this style */
    std::shared_ptr<VertexLayout> m_vertexLayout;

    /* <Material> used for drawing meshes that use this style */
    std::shared_ptr<Material> m_material = std::make_shared<Material>();

    /* <LightingType> to determine how lighting will be calculated for this style */
    LightingType m_lightingType = LightingType::fragment;

    /* Draw mode to pass into <VboMesh>es created with this style */
    GLenum m_drawMode;

    /* vector of SceneLayers a style can operator on */
    /* TODO: decouple layers and styles so that sublayers can apply different styles than the parent */
    std::vector<std::shared_ptr<Tangram::SceneLayer>> m_layers;

    /* Create <VertexLayout> corresponding to this style; subclasses must implement this and call it on construction */
    virtual void constructVertexLayout() = 0;

    /* Create <ShaderProgram> for this style; subclasses must implement this and call it on construction */
    virtual void constructShaderProgram() = 0;

    /* Build styled vertex data for point geometry and add it to the given <VboMesh> */
    virtual void buildPoint(Point& _point, const StyleParamMap& _styleParamMap, Properties& _props, VboMesh& _mesh) const = 0;

    /* Build styled vertex data for line geometry and add it to the given <VboMesh> */
    virtual void buildLine(Line& _line, const StyleParamMap& _styleParamMap, Properties& _props, VboMesh& _mesh) const = 0;

    /* Build styled vertex data for polygon geometry and add it to the given <VboMesh> */
    virtual void buildPolygon(Polygon& _polygon, const StyleParamMap& _styleParamMap, Properties& _props, VboMesh& _mesh) const = 0;

    static std::unordered_map<std::bitset<MAX_LAYERS>, StyleParamMap> s_styleParamMapCache;
    static std::mutex s_cacheMutex;
    static uint32_t parseColorProp(const std::string& _colorPropStr) ;

    /*
     * filter what layer(s) a features match and get style paramaters for this feature based on all subLayers it
     * matches. Matching is cached for other features to use.
     * Parameter maps for a set of layers is determined by merging parameters maps for individual layers matching the
     * filters and keyed based on a uniqueID defined by the id of the matching layers.
     */
    void applyLayerFiltering(const Feature& _feature, const Tangram::Context& _ctx, std::bitset<MAX_LAYERS>& _uniqueID,
                                        StyleParamMap& _styleParamMapMix, std::shared_ptr<Tangram::SceneLayer> _uberLayer) const;

    /* Perform any needed setup to process the data for a tile */
    virtual void onBeginBuildTile(MapTile& _tile) const;

    /* Perform any needed teardown after processing data for a tile */
    virtual void onEndBuildTile(MapTile& _tile, std::shared_ptr<VboMesh> _mesh) const;

    /* Create a new mesh object using the vertex layout corresponding to this style */
    virtual VboMesh* newMesh() const = 0;

public:

    Style(std::string _name, GLenum _drawMode);

    virtual ~Style();

    /* Make this style ready to be used (call after all needed properties are set) */
    virtual void build(const std::vector<std::unique_ptr<Light>>& _lights);

    /* Add layers to which this style will apply */
    void addLayer(std::shared_ptr<Tangram::SceneLayer> _layer);

    /* Add styled geometry from the given <TileData> object to the given <MapTile> */
    virtual void addData(TileData& _data, MapTile& _tile);

    /* Perform any setup needed before drawing each frame */
    virtual void onBeginDrawFrame(const std::shared_ptr<View>& _view, const std::shared_ptr<Scene>& _scene);

    /* Perform any unsetup needed after drawing each frame */
    virtual void onEndDrawFrame() {}

    virtual void setLightingType(LightingType _lType);

    void setMaterial(const std::shared_ptr<Material>& _material);

    void setPixelScale(float _pixelScale) { m_pixelScale = _pixelScale; }

    std::shared_ptr<Material> getMaterial() { return m_material; }

    std::shared_ptr<ShaderProgram> getShaderProgram() const { return m_shaderProgram; }

    const std::string& getName() const { return m_name; }

};


typedef std::vector<std::unique_ptr<Style>> StyleSet;
