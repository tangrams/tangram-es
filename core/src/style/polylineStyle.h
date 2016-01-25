#pragma once

#include "style.h"
#include "glm/vec2.hpp"
// TODO move cap/join types to types.h
#include "util/builders.h"

namespace Tangram {

class PolylineStyle : public Style {

protected:

    struct Parameters {
        uint32_t order = 0;
        uint32_t outlineOrder = 0;
        uint32_t color = 0xff00ffff;
        uint32_t outlineColor = 0xff00ffff;
        CapTypes cap = CapTypes::butt;
        CapTypes outlineCap = CapTypes::butt;
        JoinTypes join = JoinTypes::miter;
        JoinTypes outlineJoin = JoinTypes::miter;
        bool outlineOn = false;
        glm::vec2 extrude;
    };

    virtual void constructVertexLayout() override;
    virtual void constructShaderProgram() override;
    virtual void buildLine(const Line& _line, const DrawRule& _rule, const Properties& _props, VboMesh& _mesh, Tile& _tile) const override;
    virtual void buildPolygon(const Polygon& _poly, const DrawRule& _rule, const Properties& _props, VboMesh& _mesh, Tile& _tile) const override;

    Parameters parseRule(const DrawRule& _rule) const;

    virtual VboMesh* newMesh() const override;

public:

    PolylineStyle(std::string _name, Blending _blendMode = Blending::none, GLenum _drawMode = GL_TRIANGLES);

    virtual ~PolylineStyle() {
    }
};

}
