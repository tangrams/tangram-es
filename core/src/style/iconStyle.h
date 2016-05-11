#pragma once

#include "iconStyle.h"

#include "style/textStyle.h"
#include "style/pointStyle.h"

#include "style.h"

#include <memory>
#include <string>

namespace Tangram {

struct IconMesh : LabelSet {

    std::unique_ptr<StyledMesh> textLabels;
    std::unique_ptr<StyledMesh> spriteLabels;

    void addLabels(std::vector<std::unique_ptr<Label>>& _labels) {
    typedef std::vector<std::unique_ptr<Label>>::iterator iter_t;
    m_labels.insert(m_labels.end(),
                    std::move_iterator<iter_t>(_labels.begin()),
                    std::move_iterator<iter_t>(_labels.end()));
}

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
