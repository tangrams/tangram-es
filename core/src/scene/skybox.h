#pragma once

#include "gl/typedMesh.h"

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

    typedef TypedMesh<PosVertex> Mesh;
    std::unique_ptr<Mesh> m_mesh;

    std::string m_file;

};

}