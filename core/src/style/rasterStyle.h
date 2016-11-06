#pragma once

#include "style.h"
#include "polygonStyle.h"

namespace Tangram {

class RasterStyle : public PolygonStyle {

protected:

    virtual void constructShaderProgram() override;

public:

    RasterStyle(std::string _name, Blending _blendMode = Blending::opaque, GLenum _drawMode = GL_TRIANGLES);

};

}
