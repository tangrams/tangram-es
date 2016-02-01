#pragma once

#include "gl.h"
#include "data/tileData.h"
#include "util/uniform.h"

#include <memory>
#include <string>
#include <vector>

namespace Tangram {

struct DrawRule;
class Light;
class Tile;
class MapProjection;
class Material;
class VertexLayout;
class View;
class Scene;
class ShaderProgram;
class Style;

enum class LightingType : char {
    none,
    vertex,
    fragment
};

enum class Blending : int8_t {
    none = 0,
    add,
    multiply,
    inlay,
    overlay,
};

struct StyledMesh {
    virtual void draw(ShaderProgram& _shader) = 0;
    virtual size_t bufferSize() = 0;

    virtual ~StyledMesh() {}

};

class StyleBuilder {
public:

    StyleBuilder(const Style& _style);

    virtual void setup(const Tile& _tile) = 0;

    virtual void addFeature(const Feature& _feat, const DrawRule& _rule);

    /* Build styled vertex data for point geometry */
    virtual void addPoint(const Point& _point, const Properties& _props, const DrawRule& _rule);

    /* Build styled vertex data for line geometry */
    virtual void addLine(const Line& _line, const Properties& _props, const DrawRule& _rule);

    /* Build styled vertex data for polygon geometry */
    virtual void addPolygon(const Polygon& _polygon, const Properties& _props, const DrawRule& _rule);

    /* Create a new mesh object using the vertex layout corresponding to this style */
    virtual std::unique_ptr<StyledMesh> build() = 0;

    virtual bool checkRule(const DrawRule& _rule) const;

    virtual const Style& style() const = 0;

protected:
    bool m_hasColorShaderBlock = false;
};

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

using StyleUniform = std::pair< std::string, UniformValue >;

protected:

    /* The platform pixel scale */
    float m_pixelScale = 1.0;

    /* Unique name for a style instance */
    std::string m_name;
    uint32_t m_id;

    /* <ShaderProgram> used to draw meshes using this style */
    std::unique_ptr<ShaderProgram> m_shaderProgram;

    /* <VertexLayout> shared between meshes using this style */
    std::shared_ptr<VertexLayout> m_vertexLayout;

    /* <Material> used for drawing meshes that use this style */
    std::shared_ptr<Material> m_material;

    /* <LightingType> to determine how lighting will be calculated for this style */
    LightingType m_lightingType = LightingType::fragment;

    Blending m_blend = Blending::none;
    int m_blendOrder = -1;

    /* Draw mode to pass into <Mesh>es created with this style */
    GLenum m_drawMode;

    /* animated property */
    bool m_animated = false;

    /* Create <VertexLayout> corresponding to this style; subclasses must
     * implement this and call it on construction
     */
    virtual void constructVertexLayout() = 0;

    /* Create <ShaderProgram> for this style; subclasses must implement
     * this and call it on construction
     */
    virtual void constructShaderProgram() = 0;

    /* Set uniform values when @_updateUniforms is true,
     * and bind textures starting at @_textureUnit
     */
    void setupShaderUniforms(int _textureUnit, Scene& _scene);

private:

    std::vector<StyleUniform> m_styleUniforms;

public:

    Style(std::string _name, Blending _blendMode, GLenum _drawMode);

    virtual ~Style();

    static bool compare(std::unique_ptr<Style>& a, std::unique_ptr<Style>& b) {

        const auto& modeA = a->blendMode();
        const auto& modeB = b->blendMode();
        const auto& orderA = a->blendOrder();
        const auto& orderB = b->blendOrder();

        if (modeA != Blending::none && modeB != Blending::none) {
            if (orderA != orderB) {
                return orderA < orderB;
            }
        }
        if (modeA != modeB) {
            return static_cast<uint8_t>(modeA) < static_cast<uint8_t>(modeB);
        }
        return a->getName() < b->getName();
    }

    Blending blendMode() const { return m_blend; };
    int blendOrder() const { return m_blendOrder; };

    void setBlendMode(Blending _blendMode) { m_blend = _blendMode; }
    void setBlendOrder(int _blendOrder) { m_blendOrder = _blendOrder; }

    /* Whether or not the style is animated */
    bool isAnimated() { return m_animated; }

    /* Make this style ready to be used (call after all needed properties are set) */
    virtual void build(const std::vector<std::unique_ptr<Light>>& _lights);

    /* Perform any setup needed before drawing each frame
     * _textUnit is the next available texture unit
     */
    virtual void onBeginDrawFrame(const View& _view, Scene& _scene, int _textureUnit = 0);

    /* Perform any unsetup needed after drawing each frame */
    virtual void onEndDrawFrame() {}

    virtual void setLightingType(LightingType _lType);

    void setAnimated(bool _animated) { m_animated = _animated; }

    void setMaterial(const std::shared_ptr<Material>& _material);

    void setPixelScale(float _pixelScale) { m_pixelScale = _pixelScale; }

    void setID(uint32_t _id) { m_id = _id; }

    std::shared_ptr<Material> getMaterial() { return m_material; }

    const std::unique_ptr<ShaderProgram>& getShaderProgram() const { return m_shaderProgram; }

    const std::string& getName() const { return m_name; }
    const uint32_t& getID() const { return m_id; }

    std::vector<StyleUniform>& styleUniforms() { return m_styleUniforms; }

    virtual std::unique_ptr<StyleBuilder> createBuilder() const = 0;

    GLenum drawMode() const { return m_drawMode; }
    float pixelScale() const { return m_pixelScale; }
    const auto& vertexLayout() const { return m_vertexLayout; }

};

}
