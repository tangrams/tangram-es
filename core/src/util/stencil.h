#include "gl.h"
#include "util/shaderProgram.h"
#include "util/typedMesh.h"

#include "glm/mat4x4.hpp"

struct VertexLayout;

class StencilClipper {
public:
    StencilClipper();

    void draw(glm::mat4 modelViewProjMatrix);

private:
    std::shared_ptr<VertexLayout> m_vertexLayout;
    ShaderProgram m_shader;

    TypedMesh<glm::vec2> m_mesh;

};
