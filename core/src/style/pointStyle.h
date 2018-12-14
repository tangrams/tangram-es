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

    constexpr static float DEFAULT_PLACEMENT_SPACING = 80.f;

    PointStyle(std::string _name, Blending _blendMode = Blending::overlay,
               GLenum _drawMode = GL_TRIANGLES, bool _selection = true);

    virtual ~PointStyle();

    virtual void onBeginUpdate() override;
    virtual void onBeginDrawFrame(RenderState& rs, const View& _view) override;
    virtual void onBeginFrame(RenderState& rs) override;
    virtual void onBeginDrawSelectionFrame(RenderState& rs, const View& _view) override;
    virtual bool draw(RenderState& rs, const Tile& _tile) override { return false; }
    virtual bool draw(RenderState& rs, const Marker& _marker) override { return false; }

    void setFontContext(FontContext& _fontContext) {
        if (m_textStyle) { m_textStyle->setFontContext(_fontContext); }
    }

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

    SpriteVertex* pushQuad(Texture* texture) const;

protected:

    void drawMesh(RenderState& rs, ShaderProgram& shaderProgram, UniformLocation& uSpriteMode);

    std::shared_ptr<Texture> m_defaultTexture;
    const std::unordered_map<std::string, std::shared_ptr<Texture>>* m_textures = nullptr;

    struct UniformBlock {
        UniformLocation uTex{"u_tex"};
        UniformLocation uOrtho{"u_ortho"};
        UniformLocation uSpriteMode{"u_sprite_mode"};
    } m_mainUniforms, m_selectionUniforms;

    struct TextureBatch {
        TextureBatch(Texture* t) : texture(t) {}
        Texture* texture = nullptr;
        size_t vertexCount = 0;
    };

    mutable std::unique_ptr<DynamicQuadMesh<SpriteVertex>> m_mesh;
    mutable std::vector<TextureBatch> m_batches;

    std::unique_ptr<TextStyle> m_textStyle;
};

}
