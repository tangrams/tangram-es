#pragma once

#include "style.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "labels/label.h"

namespace Tangram {

class Texture;
class SpriteAtlas;

class PointStyle : public Style {

protected:

    struct Parameters {
        bool centroid = false;
        std::string sprite;
        std::string spriteDefault;
        glm::vec2 size;
        uint32_t color = 0xffffffff;
        bool valid = true;
        Label::Options labelOptions;
        float extrudeScale = 1.f;
    };

    virtual void constructVertexLayout() override;
    virtual void constructShaderProgram() override;
    virtual void buildPoint(const Point& _point, const DrawRule& _rule, const Properties& _props, VboMesh& _mesh, Tile& _tile) const override;
    virtual void buildLine(const Line& _line, const DrawRule& _rule, const Properties& _props, VboMesh& _mesh, Tile& _tile) const override;
    virtual void buildPolygon(const Polygon& _polygon, const DrawRule& _rule, const Properties& _props, VboMesh& _mesh, Tile& _tile) const override;

    void pushQuad(std::vector<Label::Vertex>& _vertices, const glm::vec2& _size, const glm::vec2& _uvBL,
            const glm::vec2& _uvTR, unsigned int _color, float _extrudeScale) const;
    bool getUVQuad(Parameters& _params, glm::vec4& _quad) const;

    Parameters applyRule(const DrawRule& _rule, const Properties& _props, float _zoom) const;

    virtual VboMesh* newMesh() const override;

    std::shared_ptr<SpriteAtlas> m_spriteAtlas;
    std::shared_ptr<Texture> m_texture;

public:

    virtual void onBeginDrawFrame(const View& _view, Scene& _scene, int _textureUnit = 0) override;

    PointStyle(std::string _name, Blending _blendMode = Blending::overlay, GLenum _drawMode = GL_TRIANGLES);
    void setSpriteAtlas(std::shared_ptr<SpriteAtlas> _spriteAtlas) { m_spriteAtlas = _spriteAtlas; }
    void setTexture(std::shared_ptr<Texture> _texture) { m_texture = _texture; }

    virtual ~PointStyle();

};

}
