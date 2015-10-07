#pragma once

#include "style.h"

#include <mutex>
#include <tuple>

namespace Tangram {

class PolygonStyle : public Style {

protected:

    struct Parameters {
        uint32_t order = 0;
        uint32_t color = 0xff00ffff;
        glm::vec2 extrude;
    };

    virtual void constructVertexLayout() override;
    virtual void constructShaderProgram() override;
    virtual void buildPolygon(const Polygon& _polygon, const DrawRule& _rule, const Properties& _props, VboMesh& _mesh, Tile& _tile) const override;

    Parameters parseRule(const DrawRule& _rule) const;

    virtual VboMesh* newMesh() const override;

public:

    PolygonStyle(std::string _name, Blending _blendMode = Blending::none, GLenum _drawMode = GL_TRIANGLES);

    virtual ~PolygonStyle() {
    }
};

}
