#pragma once

#include "style.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "gl/typedMesh.h"
#include "labels/labelMesh.h"
#include "labels/label.h"
#include "scene/spriteAtlas.h"

namespace Tangram {

class Texture;


class SpriteStyle : public Style {

protected:

    struct Parameters {
        std::string sprite;
        std::pair<float, float> offset;
        glm::vec2 size;
        int32_t priority = std::numeric_limits<int32_t>::max();
    };

    virtual void constructVertexLayout() override;
    virtual void constructShaderProgram() override;
    virtual void buildPoint(const Point& _point, const DrawRule& _rule, const Properties& _props, VboMesh& _mesh, Tile& _tile) const override;

    Parameters parseRule(const DrawRule& _rule) const;
    
    virtual VboMesh* newMesh() const override {
        return new LabelMesh(m_vertexLayout, m_drawMode);
    };

    std::shared_ptr<SpriteAtlas> m_spriteAtlas;

public:

    virtual void onBeginDrawFrame(const View& _view, const Scene& _scene) override;

    SpriteStyle(std::string _name, Blending _blendMode = Blending::overlay, GLenum _drawMode = GL_TRIANGLES);
    void setSpriteAtlas(std::shared_ptr<SpriteAtlas> _spriteAtlas) { m_spriteAtlas = _spriteAtlas; }

    virtual ~SpriteStyle();

};

}
