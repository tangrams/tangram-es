#include "button.h"

#include "context.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "util/vertexLayout.h"
#include "util/geom.h"

void Button::init(){

    std::string frag =
"#ifdef GL_ES\n"
"precision mediump float;\n"
"#endif\n";

    // Fixed Vertex Shader
    std::string fixVertShader = frag;
    fixVertShader += STRINGIFY(
    uniform mat4 u_modelViewProjectionMatrix;
    attribute vec4 a_position;
    varying vec4 v_position;
    varying float v_alpha;
    void main(void) {
        v_position = a_position;
        v_position.z = 0.1;
        v_alpha = 1.0;
        gl_Position = u_modelViewProjectionMatrix * v_position;
    });

    // Universal Fragment Shader
    frag += STRINGIFY(
    uniform vec2 u_resolution;
    uniform vec4 u_mask;
    uniform vec4 u_color;
    varying float v_alpha;
    void main(void) {
        vec2 st = gl_FragCoord.xy;
        vec2 mask = step(u_mask.xy,st)*(1.0-step(u_mask.zw,st));
        gl_FragColor = vec4(vec3(1.0),v_alpha)*(mask.x*mask.y);
    });

    m_fixShader = std::shared_ptr<ShaderProgram>(new ShaderProgram());
    m_fixShader->setSourceStrings(frag, fixVertShader);

    {
        float cornersWidth = 10.;
        float crossWidth = 5.;

        std::shared_ptr<VertexLayout> vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
            {"a_position", 2, GL_FLOAT, false, 0}
        }));
        std::vector<LineVertex> vertices;
        std::vector<int> indices;

        // Cross
        glm::vec3 center = getCenter();
        vertices.push_back({ center.x-crossWidth, center.y});
        vertices.push_back({ center.x+crossWidth, center.y});
        vertices.push_back({ center.x, center.y-crossWidth});
        vertices.push_back({ center.x, center.y+crossWidth});
        
        indices.push_back(0); indices.push_back(1); 
        indices.push_back(2); indices.push_back(3);

        // Coorners
        glm::vec3 A = getTopLeft();
        glm::vec3 B = getTopRight();
        glm::vec3 C = getBottomRight();
        glm::vec3 D = getBottomLeft();

        vertices.push_back({ A.x, A.y + cornersWidth});
        vertices.push_back({ A.x, A.y});
        vertices.push_back({ A.x + cornersWidth, A.y});

        vertices.push_back({ B.x - cornersWidth, B.y});
        vertices.push_back({ B.x, B.y});
        vertices.push_back({ B.x, B.y + cornersWidth});

        vertices.push_back({ C.x, C.y - cornersWidth});
        vertices.push_back({ C.x, C.y});
        vertices.push_back({ C.x - cornersWidth, C.y});

        vertices.push_back({ D.x + cornersWidth, D.y});
        vertices.push_back({ D.x, D.y});
        vertices.push_back({ D.x, D.y - cornersWidth});

        indices.push_back(4); indices.push_back(5);
        indices.push_back(5); indices.push_back(6);
        indices.push_back(7); indices.push_back(8);
        indices.push_back(8); indices.push_back(9);
        indices.push_back(10); indices.push_back(11);
        indices.push_back(11); indices.push_back(12);
        indices.push_back(13); indices.push_back(14);
        indices.push_back(14); indices.push_back(15);

        std::shared_ptr<HudMesh> mesh(new HudMesh(vertexLayout, GL_LINES));
        mesh->addVertices(std::move(vertices), std::move(indices));
        mesh->compileVertexBuffer();

        m_fixMesh = mesh;
    } 
}

void Button::draw(){
    m_fixShader->use();
    m_fixShader->setUniformf("u_mask", 0, 0, getWindowWidth(), getWindowHeight());
    m_fixShader->setUniformMatrix4f("u_modelViewProjectionMatrix", glm::value_ptr(getOrthoMatrix()));
    m_fixMesh->draw(m_fixShader);
}