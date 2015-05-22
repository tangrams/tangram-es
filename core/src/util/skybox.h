#pragma once

#include <string>
#include <memory>
#include "util/textureCube.h"
#include "util/shaderProgram.h"
#include "util/typedMesh.h"
#include "view/view.h"
#include "glm/gtc/type_ptr.hpp"

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

    std::shared_ptr<ShaderProgram> m_skyboxShader;
    std::shared_ptr<Texture> m_skyboxTexture;

    typedef TypedMesh<PosVertex> Mesh;
    std::shared_ptr<Mesh> m_skyboxMesh;

    std::string m_file;

};

