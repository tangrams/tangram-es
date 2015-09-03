#pragma once

#include "style.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "gl/typedMesh.h"
#include "labels/labelMesh.h"
#include "labels/label.h"
#include "labels/spriteAtlas.h"

namespace Tangram {

class Texture;

class SpriteStyle : public Style {

protected:

    virtual void constructVertexLayout() override;
    virtual void constructShaderProgram() override;
    virtual void buildPoint(const Point& _point, const DrawRule& _rule, const Properties& _props, VboMesh& _mesh, Tile& _tile) const override;

    virtual VboMesh* newMesh() const override {
        return new LabelMesh(m_vertexLayout, m_drawMode);
    };

    std::unique_ptr<SpriteAtlas> m_spriteAtlas;

public:

    bool isOpaque() const override { return false; }

    virtual void onBeginDrawFrame(const View& _view, const Scene& _scene) override;

    SpriteStyle(std::string _name, Blending _blendMode = Blending::overlay, GLenum _drawMode = GL_TRIANGLES);

    virtual ~SpriteStyle();

};

}
