#pragma once

#include "gl/vboMesh.h"

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
    void draw(const View& _view);

private:

    struct PosVertex {
        // Position Data
        GLfloat pos_x;
        GLfloat pos_y;
        GLfloat pos_z;
    };

    std::unique_ptr<ShaderProgram> m_shader;
    std::unique_ptr<Texture> m_texture;

    std::unique_ptr<VboMesh<PosVertex>> m_mesh;

    std::string m_file;

};

}
