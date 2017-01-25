#include "rasterStyle.h"

#include "gl/shaderProgram.h"
#include "gl/mesh.h"

namespace Tangram {

RasterStyle::RasterStyle(std::string _name, Blending _blendMode, GLenum _drawMode)
    : PolygonStyle(_name, _blendMode, _drawMode, false) {

    m_rasterType = RasterType::color;
}

void RasterStyle::constructShaderProgram() {
    PolygonStyle::constructShaderProgram();
}

}
