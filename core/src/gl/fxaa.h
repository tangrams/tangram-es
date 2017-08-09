#pragma once

#include "gl/renderState.h"
#include "gl/shaderProgram.h"
#include "gl/texture.h"
#include "gl/vertexLayout.h"

namespace Tangram {

struct PostProcessEffect {
    virtual void draw(RenderState& _rs, Texture& _tex, glm::vec2 _dim) = 0;
};

class Fxaa : PostProcessEffect {
public:

    Fxaa();
    ~Fxaa();

    void draw(RenderState& _rs, Texture& _tex, glm::vec2 _dim) override;

    std::unique_ptr<ShaderProgram> m_shader;
    std::unique_ptr<VertexLayout> m_vertexLayout;

    UniformLocation m_uProj{"u_proj"};
    UniformLocation m_uResolution{"u_resolution"};
};

}
