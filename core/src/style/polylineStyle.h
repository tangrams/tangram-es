#pragma once

#include "style/style.h"

namespace Tangram {

class Texture;

class PolylineStyle : public Style {

public:

    PolylineStyle(std::string _name, Blending _blendMode = Blending::opaque, GLenum _drawMode = GL_TRIANGLES, bool _selection = true);

    virtual void constructVertexLayout() override;
    virtual void constructShaderProgram() override;
    virtual std::unique_ptr<StyleBuilder> createBuilder() const override;
    virtual void onBeginDrawFrame(RenderState& rs, const View& _view, Scene& _scene) override;
    virtual ~PolylineStyle() {}

    void setDashArray(std::vector<float> _dashArray) { m_dashArray = _dashArray; }
    void setTexture(std::shared_ptr<Texture>& _texture) { m_texture = _texture; }

    void setDashBackgroundColor(const glm::vec4 _dashBackgroundColor);

private:

    std::vector<float> m_dashArray;
    std::shared_ptr<Texture> m_texture;
    bool m_dashBackground = false;
    glm::vec4 m_dashBackgroundColor;

    UniformLocation m_uTexture{"u_texture"};
    UniformLocation m_uTextureRatio{"u_texture_ratio"};
};

}
