#pragma once

#include "style/style.h"

#include <mutex>
#include <tuple>

namespace Tangram {

class PolygonStyle : public Style {

public:

    PolygonStyle(std::string _name, Blending _blendMode = Blending::opaque, GLenum _drawMode = GL_TRIANGLES, bool _selection = true);

    virtual void constructVertexLayout() override;
    virtual void constructShaderProgram() override;
    virtual std::unique_ptr<StyleBuilder> createBuilder() const override;
    virtual ~PolygonStyle() {}

};

}
