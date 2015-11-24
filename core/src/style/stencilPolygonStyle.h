#pragma once

#include "polygonStyle.h"

namespace Tangram {

class StencilPolygonStyle : public PolygonStyle {

protected:

    virtual void constructShaderProgram() override;

public:

    StencilPolygonStyle(std::string _name, Blending _blendMode = Blending::stencil, GLenum _drawMode = GL_TRIANGLES);

    virtual ~StencilPolygonStyle() {}

    virtual bool shouldBuild(const Scene& _scene) const override;

};

}
