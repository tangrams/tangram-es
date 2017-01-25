#pragma once

#include "style.h"

namespace Tangram {

class DebugStyle : public Style {

protected:

    virtual void constructVertexLayout() override;

    virtual std::unique_ptr<StyleBuilder> createBuilder() const override;

    void buildFragmentShaderSource(ShaderSource& _out) override;
    void buildVertexShaderSource(ShaderSource& _out, bool _selectionPass) override;

public:

    DebugStyle(std::string _name, Blending _blendMode = Blending::overlay, GLenum _drawMode = GL_LINE_LOOP);

    virtual ~DebugStyle() {
    }

};

}
