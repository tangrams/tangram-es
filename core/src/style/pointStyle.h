#pragma once

#include "gl/dynamicQuadMesh.h"
#include "labels/spriteLabel.h"
#include "labels/labelProperty.h"
#include "labels/textLabels.h"
#include "style/style.h"
#include "style/textStyle.h"

#include "glm/vec3.hpp"
#include "glm/vec2.hpp"

namespace Tangram {

class Texture;
class SpriteAtlas;

class PointStyle : public Style {

public:

    struct Parameters {
        bool interactive = false;
        bool keepTileEdges = false;
        bool autoAngle = false;
        bool dynamicTexture = false;
        std::string sprite;
        std::string spriteDefault;
        std::string texture;
        glm::vec2 size;
        uint32_t color = 0xffffffff;
        Label::Options labelOptions;
        LabelProperty::Placement placement = LabelProperty::Placement::vertex;
        float extrudeScale = 1.f;
        float placementMinLengthRatio = 1.0f;
        float placementSpacing = 80.f;
    };

    PointStyle(std::string _name, std::shared_ptr<FontContext> _fontContext,
               Blending _blendMode = Blending::overlay, GLenum _drawMode = GL_TRIANGLES, bool _selection = true);

    virtual ~PointStyle();

    virtual void onBeginUpdate() override;
    virtual void onBeginDrawFrame(RenderState& rs, const View& _view, Scene& _scene) override;
    virtual void onBeginFrame(RenderState& rs) override;
    virtual void onBeginDrawSelectionFrame(RenderState& rs, const View& _view, Scene& _scene) override;
    virtual void draw(RenderState& rs, const Tile& _tile) override {}
    virtual void draw(RenderState& rs, const Marker& _marker) override {}

    void setTextures(const std::unordered_map<std::string, std::shared_ptr<Texture>>& _textures) {
        m_textures = &_textures;
    }
    void setDefaultTexture(std::shared_ptr<Texture>& _texture) {
        m_defaultTexture = _texture;
    }

    auto textures() const { return m_textures; }
    const auto& defaultTexture() const { return m_defaultTexture; }

    auto& mesh() const { return m_mesh; }
    virtual size_t dynamicMeshSize() const override { return m_mesh->bufferSize(); }

    virtual std::unique_ptr<StyleBuilder> createBuilder() const override;

    virtual void build(const Scene& _scene) override;

    virtual void constructVertexLayout() override;
    virtual void constructShaderProgram() override;

    TextStyle& textStyle() const { return *m_textStyle; }
    virtual void setPixelScale(float _pixelScale) override;

protected:

    std::shared_ptr<Texture> m_defaultTexture;
    const std::unordered_map<std::string, std::shared_ptr<Texture>>* m_textures = nullptr;

    struct UniformBlock {
        UniformLocation uTex{"u_tex"};
        UniformLocation uOrtho{"u_ortho"};
    } m_mainUniforms, m_selectionUniforms;

    mutable std::unique_ptr<DynamicQuadMesh<SpriteVertex>> m_mesh;

    std::unique_ptr<TextStyle> m_textStyle;
};

}

namespace std {
    template <>
    struct hash<Tangram::PointStyle::Parameters> {
        size_t operator() (const Tangram::PointStyle::Parameters& p) const {
            std::hash<Tangram::Label::Options> optionsHash;
            std::size_t seed = 0;
            hash_combine(seed, p.sprite);
            hash_combine(seed, p.color);
            hash_combine(seed, p.size.x);
            hash_combine(seed, p.size.y);
            hash_combine(seed, (int)p.placement);
            hash_combine(seed, optionsHash(p.labelOptions));
            return seed;
        }
    };
}
