#pragma once

#include "style.h"
#include "polygonStyle.h"

namespace Tangram {

class RasterStyle : public PolygonStyle {

protected:

    virtual void constructShaderProgram() override;

    virtual bool hasRasters() const override { return true; }

public:

    RasterStyle(std::string _name, Blending _blendMode = Blending::none, GLenum _drawMode = GL_TRIANGLES);

};

}
