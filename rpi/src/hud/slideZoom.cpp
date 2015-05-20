#include "slideZoom.h"

#include "context.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "util/vertexLayout.h"
#include "util/geom.h"

void SlideZoom::init(){

    zoom = 0;
    slider = 0;

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
    void main(void) {
        v_position = a_position;
        v_position.z = 0.1;
        gl_Position = u_modelViewProjectionMatrix * v_position;
    });

    // Translatation Vertex Shader
    std::string trnVertShader = frag;
    trnVertShader += STRINGIFY(
    uniform mat4 u_modelViewProjectionMatrix;
    uniform vec2 u_offset;
    attribute vec4 a_position;
    varying vec4 v_position;
    void main(void) {
        v_position = vec4(u_offset.x,u_offset.y,0.1,0.0) + a_position;
        gl_Position = u_modelViewProjectionMatrix * v_position;
    });

    // Universal Fragment Shader
    frag += STRINGIFY(
    uniform vec2 u_resolution;
    uniform vec4 u_mask;
    uniform vec4 u_color;
    void main(void) {
        vec2 st = gl_FragCoord.xy;
        vec2 mask = step(u_mask.xy,st)*(1.0-step(u_mask.zw,st));
        gl_FragColor = vec4(1.0)*(mask.x*mask.y);
    });

    m_fixShader = std::shared_ptr<ShaderProgram>(new ShaderProgram());
    m_fixShader->setSourceStrings(frag, fixVertShader);

    m_trnShader = std::shared_ptr<ShaderProgram>(new ShaderProgram());
    m_trnShader->setSourceStrings(frag, trnVertShader);

    m_verticalRulerMeshA = getVerticalRulerMesh(-getWindowHeight(),getWindowHeight(),5.0f,getWindowWidth()*0.0131125);
    m_verticalRulerMeshB = getVerticalRulerMesh(-getWindowHeight(),getWindowHeight(),50.0f,getWindowWidth()*0.0194525);

    m_triangle = getTriangle(getWindowHeight()*0.005);

    // Make fixed lines
    {
        std::shared_ptr<VertexLayout> vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
            {"a_position", 2, GL_FLOAT, false, 0}
        }));
        std::vector<LineVertex> vertices;
        std::vector<int> indices;

        Rectangle bBox = Rectangle(*this, 5.f);
        glm::vec3 a = bBox.getTopRight();
        glm::vec3 b = bBox.getTopLeft();
        glm::vec3 c = bBox.getBottomLeft();
        glm::vec3 d = bBox.getBottomRight();

        vertices.push_back({ a.x, a.y});
        vertices.push_back({ b.x, b.y});
        vertices.push_back({ c.x, c.y});
        vertices.push_back({ d.x, d.y});
        
        indices.push_back(0); indices.push_back(1);
        indices.push_back(1); indices.push_back(2); 
        indices.push_back(2); indices.push_back(3);

        vertices.push_back({ b.x - 10.0, getWindowHeight()*0.5-20.0 });
        vertices.push_back({ b.x - 5.0, getWindowHeight()*0.5-20.0 });
        indices.push_back(4); indices.push_back(5);

        vertices.push_back({ b.x - 10.0, getWindowHeight()*0.5 });
        vertices.push_back({ b.x - 5.0, getWindowHeight()*0.5 });        
        indices.push_back(6); indices.push_back(7); 

        vertices.push_back({ b.x - 10.0, getWindowHeight()*0.5+20.0 });
        vertices.push_back({ b.x - 5.0, getWindowHeight()*0.5+20.0 });        
        indices.push_back(8); indices.push_back(9); 

        std::shared_ptr<HudMesh> mesh(new HudMesh(vertexLayout, GL_LINES));
        mesh->addVertices(std::move(vertices), std::move(indices));
        mesh->compileVertexBuffer();

        m_fixed = mesh;
    }
}

void SlideZoom::draw(){

    if ( slider > height ) {
        slider -= height;
    } else if ( slider < -height ) {
        slider += height;
    }

    m_trnShader->use();
    m_trnShader->setUniformf("u_offset", x, slider);
    m_trnShader->setUniformf("u_mask", x, y, x+width, y+height);
    m_trnShader->setUniformMatrix4f("u_modelViewProjectionMatrix", glm::value_ptr(getOrthoMatrix()));
    m_verticalRulerMeshA->draw(m_trnShader);
    m_verticalRulerMeshB->draw(m_trnShader);

    m_trnShader->use();
    m_trnShader->setUniformf("u_offset", x-25., getWindowHeight()*0.5-(zoom-9)*2.1f) ;
    m_trnShader->setUniformf("u_mask", 0, 0, getWindowWidth(), getWindowHeight());
    m_trnShader->setUniformMatrix4f("u_modelViewProjectionMatrix", glm::value_ptr(getOrthoMatrix()));
    m_triangle->draw(m_trnShader);

    m_fixShader->use();
    m_fixShader->setUniformf("u_mask", 0, 0, getWindowWidth(), getWindowHeight());
    m_fixShader->setUniformMatrix4f("u_modelViewProjectionMatrix", glm::value_ptr(getOrthoMatrix()));
    glLineWidth(2.0f);
    m_fixed->draw(m_fixShader);
    glLineWidth(1.0f);
}