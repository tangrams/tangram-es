#pragma once

#include "style.h"

#include <mutex>
#include <tuple>

namespace Tangram {

class PolygonStyle : public Style {

public:

    PolygonStyle(std::string _name, Blending _blendMode = Blending::opaque, GLenum _drawMode = GL_TRIANGLES, bool _selection = true);

    void constructVertexLayout() override;
    std::unique_ptr<StyleBuilder> createBuilder() const override;
    ~PolygonStyle() {}

    void buildFragmentShaderSource(ShaderSource& _out) override;
    void buildVertexShaderSource(ShaderSource& _out, bool _selectionPass) override;

};

}
