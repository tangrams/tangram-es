#include "stencilPolygonStyle.h"

#include "gl/shaderProgram.h"
#include "gl/typedMesh.h"
#include "scene/scene.h"

#include <memory>

namespace Tangram {

StencilPolygonStyle::StencilPolygonStyle(std::string _name, Blending _blendMode, GLenum _drawMode)
    : PolygonStyle(_name, _blendMode, _drawMode)
{
}

bool StencilPolygonStyle::shouldBuild(const Scene& _scene) const {
    return _scene.containsStyleWithBlend(Blending::inlay);
}

void StencilPolygonStyle::constructShaderProgram() {
    std::string vertShaderSrcStr = stringFromFile("shaders/stencilPolygon.vs", PathType::internal);
    std::string fragShaderSrcStr = stringFromFile("shaders/stencilPolygon.fs", PathType::internal);

    m_shaderProgram->setSourceStrings(fragShaderSrcStr, vertShaderSrcStr);
}

}
