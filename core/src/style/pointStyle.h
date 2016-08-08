#pragma once

#include "style.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "labels/spriteLabel.h"
#include "labels/labelProperty.h"
#include "gl/dynamicQuadMesh.h"
#include "style/textStyle.h"
#include "labels/textLabels.h"

namespace Tangram {

class Texture;
class SpriteAtlas;

class PointStyle : public Style {

public:

    struct Parameters {
        bool centroid = false;
        std::string sprite;
        std::string spriteDefault;
        glm::vec2 size;
        uint32_t color = 0xffffffff;
        Label::Options labelOptions;
        LabelProperty::Anchor anchor = LabelProperty::Anchor::center;
        float extrudeScale = 1.f;
    };

    PointStyle(std::string _name, std::shared_ptr<FontContext> _fontContext,
               Blending _blendMode = Blending::overlay, GLenum _drawMode = GL_TRIANGLES);

    virtual void onBeginUpdate() override;
    virtual void onBeginDrawFrame(const View& _view, Scene& _scene) override;
    virtual void onBeginFrame() override;

    void setSpriteAtlas(std::shared_ptr<SpriteAtlas> _spriteAtlas) { m_spriteAtlas = _spriteAtlas; }
    void setTexture(std::shared_ptr<Texture> _texture) { m_texture = _texture; }

    const auto& texture() const { return m_texture; }
    const auto& spriteAtlas() const { return m_spriteAtlas; }

    virtual ~PointStyle();

    auto& getMesh() const { return m_mesh; }
    virtual size_t dynamicMeshSize() const override { return m_mesh->bufferSize(); }

    virtual std::unique_ptr<StyleBuilder> createBuilder() const override;

    virtual void constructVertexLayout() override;
    virtual void constructShaderProgram() override;

    TextStyle& textStyle() const { return *m_textStyle; }
    virtual void setPixelScale(float _pixelScale) override;

protected:

    std::shared_ptr<SpriteAtlas> m_spriteAtlas;
    std::shared_ptr<Texture> m_texture;

    UniformLocation m_uTex{"u_tex"};
    UniformLocation m_uOrtho{"u_ortho"};

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
            hash_combine(seed, p.centroid);
            hash_combine(seed, p.sprite);
            hash_combine(seed, p.color);
            hash_combine(seed, (int)p.anchor);
            hash_combine(seed, p.size.x);
            hash_combine(seed, p.size.y);
            hash_combine(seed, optionsHash(p.labelOptions));
            return seed;
        }
    };
}
