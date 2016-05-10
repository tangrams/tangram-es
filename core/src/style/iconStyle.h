#pragma once

#include "iconStyle.h"

#include "style/textStyle.h"
#include "style/pointStyle.h"

#include "style.h"

#include <memory>
#include <string>

namespace Tangram {

struct IconMesh : StyledMesh {
    bool draw(ShaderProgram& _shader) override { return true; }
    size_t bufferSize() const override { return 0; }

    std::unique_ptr<StyledMesh> textLabels;
    std::unique_ptr<StyledMesh> spriteLabels;
};

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

    void setPixelScale(float _pixelScale) override;

    virtual ~IconStyle() override;

    PointStyle& pointStyle() const { return *m_pointStyle; }

    std::unique_ptr<TextStyle> m_textStyle;
    std::unique_ptr<PointStyle> m_pointStyle;
};

}
