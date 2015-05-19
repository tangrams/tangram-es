#include "hud.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "utils.h"

#define STRINGIFY(A) #A

Hud::Hud(): m_windowSize(0.0f,0.0f), m_orthoMatrix(0.0f) {

}

Hud::~Hud(){
}

void Hud::init() {

    m_cursorShader = std::make_shared<ShaderProgram>();
    m_cursorGeom = std::make_shared<Vbo>();
    m_cursorGeom.reset(cross(glm::vec3(0.,0.,-2.), 10.).getVbo()); 

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
attribute vec4 a_color;

varying vec4 v_color;
varying vec4 v_position;

void main(void) {
    v_position = vec4(u_mouse.x,u_mouse.y,0.0,0.0) + a_position;
    v_color = a_color;
    gl_Position = u_modelViewProjectionMatrix * v_position;
}

);

    frag += STRINGIFY(

uniform vec2 u_mouse;
void main(void) {
    gl_FragColor = vec4(1.0);
}
);

    m_cursorShader->setSourceStrings(frag,vert);
    m_cursorShader->build();
}

void Hud::mousePosition(int _x, int _y){
    m_mousePos.x = _x;
    m_mousePos.y = _y;
}

void Hud::mouseMove(float _x,float _y){
    m_mousePos.x += _x;
    m_mousePos.y += _y;

    if ( m_mousePos.x > m_windowSize.x){
         m_mousePos.x = 0;
    }

    if ( m_mousePos.y > m_windowSize.y){
         m_mousePos.y = 0;
    }

    if ( m_mousePos.x < 0){
         m_mousePos.x = m_windowSize.x;
    }

    if ( m_mousePos.y < 0){
         m_mousePos.y = m_windowSize.y;
    }
}

void Hud::setWindowSize(int _width, int _height){
    m_windowSize.x = _width;
    m_windowSize.y = _height;
    m_orthoMatrix = glm::ortho(0.0f, m_windowSize.x, 0.0f, m_windowSize.y);
}

void Hud::draw(){

    glPolygonOffset(-1.0f, -1.0f);
    glEnable(GL_POLYGON_OFFSET_LINE);
    glEnable(GL_LINE_SMOOTH);

    glLineWidth(2.0f);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    std::cout << m_mousePos.x << "x" << m_mousePos.y << std::endl;

    m_cursorShader->use();
    m_cursorShader->setUniformf("u_mouse", m_mousePos);
    m_cursorShader->setUniformf("u_resolution", m_windowSize);
    m_cursorShader->setUniformMatrix4f("u_modelViewProjectionMatrix", glm::value_ptr(m_orthoMatrix));
    m_cursorGeom->draw(m_cursorShader);

    glLineWidth(1.0f);
    glDisable(GL_BLEND);

    glDisable(GL_POLYGON_OFFSET_LINE);
    glDisable(GL_LINE_SMOOTH);
}