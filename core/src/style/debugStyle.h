#pragma once

#include "style/style.h"

namespace Tangram {

class DebugStyle : public Style {

protected:

    virtual void constructVertexLayout() override;
    virtual void constructShaderProgram() override;

    virtual std::unique_ptr<StyleBuilder> createBuilder() const override;

public:

    DebugStyle(std::string _name, Blending _blendMode = Blending::overlay, GLenum _drawMode = GL_LINE_LOOP);

    virtual ~DebugStyle() {
    }

};

}
