#pragma once

#include "iconStyle.h"

#include "style/textStyle.h"
#include "style/pointStyle.h"

#include "style.h"

#include <memory>
#include <string>

namespace Tangram {

class IconStyle : public Style {

protected:

    void constructVertexLayout() override;
    void constructShaderProgram() override;

    std::unique_ptr<StyleBuilder> createBuilder() const override;

public:

    IconStyle(std::string _name, Blending _blendMode = Blending::overlay, GLenum _drawMode = GL_TRIANGLES);

    void onBeginUpdate() override;
    void onBeginFrame() override;
    void onBeginDrawFrame(const View& _view, Scene& _scene) override;
    virtual ~IconStyle() override;

    std::unique_ptr<TextStyle> m_textStyle;
    std::unique_ptr<PointStyle> m_pointStyle;
};

}
