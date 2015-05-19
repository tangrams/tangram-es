#include "hud.h"

#include "context.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "util/vertexLayout.h"
#include "util/geom.h"

#include "utils.h"

#define STRINGIFY(A) #A

Hud::Hud(){
}

Hud::~Hud(){
}

void Hud::init() {

    std::string vert =
"#ifdef GL_ES\n"
"precision mediump float;\n"
"#endif\n";

    std::string frag = vert;
    vert += STRINGIFY(
    uniform mat4 u_modelViewProjectionMatrix;
    uniform vec2 u_mouse;
    uniform vec2 u_resolution;
    attribute vec4 a_position;
    varying vec4 v_position;
    void main(void) {
        v_position = vec4(u_mouse.x,u_mouse.y,0.0,0.0) + a_position;
        gl_Position = u_modelViewProjectionMatrix * v_position;
    });

    frag += STRINGIFY(
    uniform vec2 u_mouse;
    void main(void) {
        gl_FragColor = vec4(1.0);
    });

    float  mouseSize = 10.0f;
    m_cursorShader = std::shared_ptr<ShaderProgram>(new ShaderProgram());
    m_cursorShader->setSourceStrings(frag, vert );

    std::shared_ptr<VertexLayout> vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
        {"a_position", 3, GL_FLOAT, false, 0}
    }));
    std::vector<LineVertex> vertices;
    std::vector<int> indices;

    // Small billboard for the mouse
    {
        vertices.push_back({ -mouseSize, 0, 0.1 });
        vertices.push_back({ mouseSize, 0, 0.1 });
        vertices.push_back({ 0, -mouseSize, 0.1 });
        vertices.push_back({ 0, mouseSize, 0.1 });
        
        indices.push_back(0); indices.push_back(1); 
        indices.push_back(2); indices.push_back(3);
    }
    
    m_cursorMesh = std::shared_ptr<LineMesh>(new LineMesh(vertexLayout, GL_LINES));
    m_cursorMesh->addVertices(std::move(vertices), std::move(indices));
    m_cursorMesh->compileVertexBuffer();
}

void Hud::draw(){
    glLineWidth(2.0f);
    
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    m_cursorShader->use();
    m_cursorShader->setUniformf("u_mouse", getMouseX(), getMouseY());
    m_cursorShader->setUniformMatrix4f("u_modelViewProjectionMatrix", glm::value_ptr(getOrthoMatrix()));
    m_cursorMesh->draw(m_cursorShader);
    glLineWidth(1.0f);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}