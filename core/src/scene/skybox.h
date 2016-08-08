#pragma once

#include "gl/mesh.h"

#include <string>
#include <memory>

namespace Tangram {

class View;
class ShaderProgram;
class Texture;

class Skybox {

public:

    Skybox(std::string _file);

    ~Skybox() {}

    void init();
    void draw(RenderState& rs, const View& _view);

private:

    struct PosVertex {
        // Position Data
        GLfloat pos_x;
        GLfloat pos_y;
        GLfloat pos_z;
    };

    std::unique_ptr<ShaderProgram> m_shader;
    std::unique_ptr<Texture> m_texture;

    std::unique_ptr<Mesh<PosVertex>> m_mesh;

    std::string m_file;

    UniformLocation m_uModelViewProj{"u_modelViewProj"};
    UniformLocation m_uTex{"u_tex"};
};

}
