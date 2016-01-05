#pragma once

#include "style.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "labels/label.h"
#include "labelProperty.h"

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

    PointStyle(std::string _name, Blending _blendMode = Blending::overlay, GLenum _drawMode = GL_TRIANGLES);

    virtual void onBeginDrawFrame(const View& _view, Scene& _scene, int _textureUnit = 0) override;

    void setSpriteAtlas(std::shared_ptr<SpriteAtlas> _spriteAtlas) { m_spriteAtlas = _spriteAtlas; }
    void setTexture(std::shared_ptr<Texture> _texture) { m_texture = _texture; }

    const auto& texture() const { return m_texture; }
    const auto& spriteAtlas() const { return m_spriteAtlas; }

    virtual ~PointStyle();

protected:

    virtual void constructVertexLayout() override;
    virtual void constructShaderProgram() override;

    virtual std::unique_ptr<StyleBuilder> createBuilder() const override;

    std::shared_ptr<SpriteAtlas> m_spriteAtlas;
    std::shared_ptr<Texture> m_texture;

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
