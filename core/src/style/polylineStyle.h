#pragma once

#include "style.h"
// TODO move cap/join types to types.h
#include "util/builders.h"

namespace Tangram {

class PolylineStyle : public Style {

protected:

    struct Parameters {
        struct {
            uint32_t order = 0;
            uint32_t color = 0xff00ffff;
            float width = 0.f;
            float slope = 0.f;
            CapTypes cap = CapTypes::butt;
            JoinTypes join = JoinTypes::miter;
        } fill, outline;
        float height = 0.f;
        bool outlineOn = false;
    };

    virtual void constructVertexLayout() override;
    virtual void constructShaderProgram() override;
    virtual void buildLine(const Line& _line, const DrawRule& _rule, const Properties& _props, VboMesh& _mesh, Tile& _tile) const override;
    virtual void buildPolygon(const Polygon& _poly, const DrawRule& _rule, const Properties& _props, VboMesh& _mesh, Tile& _tile) const override;

    Parameters parseRule(const DrawRule& _rule, const Properties& _props, const Tile& _tile) const;
    void buildMesh(const Line& _line, Parameters& _parameters, VboMesh& _mesh) const;

    virtual VboMesh* newMesh() const override;

public:

    PolylineStyle(std::string _name, Blending _blendMode = Blending::none, GLenum _drawMode = GL_TRIANGLES);

    virtual ~PolylineStyle() {}

};

}
