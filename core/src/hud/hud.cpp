#include "hud.cpp"

#include "glm/gtc/matrix_transform.hpp"

#define STRINGIFY(A) #A

Hud::Hud(): m_windowSize(0.0f,0.0f), m_orthoMatrix(0.0f),m_cursorShader(nullptr), m_cursorGeom(nullptr){

}

Hud::~Hud(){
    if (m_cursorGeom != NULL){
        delete m_cursorGeom;
    }
}

void Hud::init() {
    m_cursorGeom = cross(glm::vec3(0.,0.,0.1), 10.).getVbo(); 

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

    m_cursorShader.load(frag,vert);
}

void Hud::mousePosition(int _x, int _y){
    m_mousePos.x = _x;
    m_mousePos.y = _y);
}

void Hud::setWindowSize(int _width, int _height){
    m_windowSize.x = _width;
    m_windowSize.y = _height;
    m_orthoMatrix = glm::ortho(0.0f, m_windowSize.x, 0.0f, m_windowSize.y);
}

void Hud::draw(){

}