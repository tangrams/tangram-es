#pragma once

#include "style.h"

namespace Tangram {

class Texture;

class PolylineStyle : public Style {

public:

    PolylineStyle(std::string _name, Blending _blendMode = Blending::none, GLenum _drawMode = GL_TRIANGLES);

    virtual void constructVertexLayout() override;
    virtual void constructShaderProgram() override;
    virtual std::unique_ptr<StyleBuilder> createBuilder() const override;
    virtual void onBeginDrawFrame(const View& _view, Scene& _scene) override;
    virtual ~PolylineStyle() {}

    void setDashArray(std::vector<int> _dashArray) { m_dashArray = _dashArray; }


private:

    std::vector<int> m_dashArray;
    std::unique_ptr<Texture> m_texture;
};

}
