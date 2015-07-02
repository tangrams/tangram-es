#pragma once

#include "util/typedMesh.h"

#include <string>
#include <memory>

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

    std::shared_ptr<ShaderProgram> m_shader;
    std::shared_ptr<Texture> m_texture;

    typedef TypedMesh<PosVertex> Mesh;
    std::shared_ptr<Mesh> m_mesh;

    std::string m_file;

};

