#include "slideRot.h"

#include "context.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "util/vertexLayout.h"
#include "util/geom.h"

void SlideRot::init(){

    angle = 0;

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

     // Rotation Vertex Shader
    std::string rotVertShader = frag;
    rotVertShader += STRINGIFY(
    uniform mat4 u_modelViewProjectionMatrix;
    uniform float u_angle;
    uniform vec2 u_resolution;
    attribute vec4 a_position;
    varying vec4 v_position;
    varying float v_alpha;
    mat2 rotate2d(float _angle){
        return mat2(cos(_angle),-sin(_angle),
                    sin(_angle),cos(_angle));
    }
    void main(void) {
        v_position =  a_position;
        v_position.xy = rotate2d(u_angle) * v_position.xy;
        v_position.xy += u_resolution.xy*0.5;
        v_position.z = 0.1;
        vec2 fromCenter = v_position.xy-u_resolution.xy*0.5;
        v_alpha = 1.0-clamp( (abs(atan(fromCenter.y,fromCenter.x)-1.57079632679)/1.57079632679)*2.0,0.0,1.0);
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

    m_rotShader = std::shared_ptr<ShaderProgram>(new ShaderProgram());
    m_rotShader->setSourceStrings(frag, rotVertShader);

    m_circularRulerMeshA = getCircularRulerMesh(((float)getWindowHeight())*0.42125,180,getWindowWidth()*0.0151125);
    m_circularRulerMeshB = getCircularRulerMesh(((float)getWindowHeight())*0.42125,36,getWindowWidth()*0.0204525);

    m_fixed = getTriangle(glm::vec2(getWindowWidth()*0.5,y+height*0.3),getWindowHeight()*0.01,PI/2.0);
}

void SlideRot::draw(){
    m_rotShader->use();
    m_rotShader->setUniformf("u_angle", angle);
    m_rotShader->setUniformf("u_mask", x, y, x+width, y+height);
    m_rotShader->setUniformf("u_resolution", (float)getWindowWidth(), (float)getWindowHeight());
    m_rotShader->setUniformMatrix4f("u_modelViewProjectionMatrix", glm::value_ptr(getOrthoMatrix()));
    m_circularRulerMeshA->draw(m_rotShader);
    glLineWidth(2.0f);
    m_circularRulerMeshB->draw(m_rotShader);
    glLineWidth(1.0f);

    m_fixShader->use();
    m_fixShader->setUniformf("u_mask", 0, 0, getWindowWidth(), getWindowHeight());
    m_fixShader->setUniformMatrix4f("u_modelViewProjectionMatrix", glm::value_ptr(getOrthoMatrix()));
    m_fixed->draw(m_fixShader);
}