#pragma once

#include "polygonStyle.h"

namespace Tangram {

class StencilPolygonStyle : public PolygonStyle {

protected:

    virtual void constructShaderProgram() override;

public:

    StencilPolygonStyle(std::string _name, Blending _blendMode = Blending::none, GLenum _drawMode = GL_TRIANGLES);

    virtual ~StencilPolygonStyle() {}

};

}
