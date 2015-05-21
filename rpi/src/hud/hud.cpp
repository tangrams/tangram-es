#include "hud.h"

#include <iostream>
#include "context.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "util/vertexLayout.h"
#include "util/geom.h"

#include "tangram.h"

#define STRINGIFY(A) #A

Hud::Hud(): m_selected(0), m_bCursor(false){
}

Hud::~Hud(){
}

void Hud::setDrawCursor(bool _true){
    m_bCursor = _true;
}

void Hud::init() {

    m_zoom.set(getWindowWidth()*0.9575,getWindowHeight()*0.30625,getWindowWidth()*0.02625,getWindowHeight()*0.3875);
    m_zoom.init();

    m_rot.set(getWindowWidth()*0.34625,getWindowHeight()*0.866667,getWindowWidth()*0.3125,getWindowHeight()*0.1);
    m_rot.init();

    if (m_bCursor){
        std::string frag =
"#ifdef GL_ES\n"
"precision mediump float;\n"
"#endif\n";

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
        uniform vec4 u_color;
        void main(void) {
            gl_FragColor = vec4(1.0);
        });

        m_trnShader = std::shared_ptr<ShaderProgram>(new ShaderProgram());
        m_trnShader->setSourceStrings(frag, trnVertShader);

        m_cursorMesh = getCrossMesh(10.0f);
    }
}

void Hud::cursorClick(float _x, float _y, int _button){
    if (m_rot.inside(_x,_y)){
        m_selected = 1;
    } else if (m_zoom.inside(_x,_y)){
        m_selected = 2;
    }
}

void Hud::cursorDrag(float _x, float _y, int _button){
    if (m_selected == 1) {

        float scale = -1.0;

        glm::vec2 screen = glm::vec2(getWindowWidth(),getWindowHeight());
        glm::vec2 mouse = glm::vec2(_x,_y)-screen*glm::vec2(0.5);
        glm::vec2 prevMouse = mouse - glm::vec2(getMouseVelX(),getMouseVelY());

        double rot1 = atan2(mouse.y, mouse.x);
        double rot2 = atan2(prevMouse.y, prevMouse.x);
        double rot = rot1-rot2;
        wrapRad(rot);
        Tangram::handleRotateGesture(getWindowWidth()/2.0, getWindowHeight()/2.0, rot*scale);

    } else if (m_selected == 2) {
        
        Tangram::handlePinchGesture(getWindowWidth()/2.0, getWindowHeight()/2.0, 1.0 + getMouseVelY()*0.01);
        m_zoom.slider += getMouseVelY();

    }
}

void Hud::cursorRelease(float _x, float _y){
    m_selected = 0;
}

bool Hud::isInUse(){
    return m_selected != 0;
}

void Hud::draw(){
    
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Cursor
    if (m_bCursor){
        m_trnShader->use();
        m_trnShader->setUniformf("u_offset", getMouseX(), getMouseY());
        m_trnShader->setUniformMatrix4f("u_modelViewProjectionMatrix", glm::value_ptr(getOrthoMatrix()));
        m_cursorMesh->draw(m_trnShader);
    }
    
    // Zoom
    m_zoom.zoom = Tangram::getZoom();
    m_zoom.draw();
    
    // Rotation
    m_rot.angle = Tangram::getRotation();
    m_rot.draw();

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}