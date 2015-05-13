#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include <sys/shm.h>

#include <stdlib.h>
#include <string.h>
#include <termios.h>

#include "context.h"
#include "tangram.h"
#include "platform.h"

#include "util/shaderProgram.h"
#include "util/vertexLayout.h"
#include "util/typedMesh.h"
#include "util/geom.h"

#define KEY_ESC      113    // q
#define KEY_ZOOM_IN  45     // - 
#define KEY_ZOOM_OUT 61     // =
#define KEY_UP       119    // w
#define KEY_LEFT     97     // a
#define KEY_RIGHT    115    // s
#define KEY_DOWN     122    // z

struct timeval tv;

static bool bUpdate = true;

// Draw Cursor
//------------------------------------------------
bool bMouse = false;
float mouseSize = 100.0f;

std::shared_ptr<ShaderProgram> mouseShader;
std::string mouseVertex = 
"precision mediump float;\n"
"uniform float u_time;\n"
"uniform vec2 u_mouse;\n"
"uniform vec2 u_resolution;\n"
"attribute vec4 a_position;\n"
"attribute vec4 a_color;\n"
"attribute vec2 a_texcoord;\n"
"varying vec4 v_color;\n"
"varying vec2 v_texcoord;\n"
"void main() {\n"
"    v_texcoord = a_texcoord;\n"
"    v_color = a_color;\n"
"    vec2 mouse = vec2(u_mouse/u_resolution)*4.0-2.0;\n"
"    gl_Position = a_position+vec4(mouse,0.0,1.0);\n"
"}\n";

std::string mouseFragment =
"precision mediump float;\n"
"uniform float u_time;\n"
"uniform vec2 u_mouse;\n"
"uniform vec2 u_resolution;\n"
"varying vec4 v_color;\n"
"varying vec2 v_texcoord;\n"
"\n"
"float box(in vec2 _st, in vec2 _size){\n"
"    _size = vec2(0.5) - _size*0.5;\n"
"    vec2 uv = smoothstep(_size, _size+vec2(0.001), _st);\n"
"    uv *= smoothstep(_size, _size+vec2(0.001), vec2(1.0)-_st);\n"
"    return uv.x*uv.y;\n"
"}\n"
"\n"
"float cross(in vec2 _st, float _size){\n"
"    return  box(_st, vec2(_size,_size/6.)) + box(_st, vec2(_size/6.,_size));\n"
"}\n"
"void main() {\n"
"    gl_FragColor = v_color * cross(v_texcoord, 0.9 );\n"
"}\n";

struct PosUVColorVertex {
    // Position Data
    GLfloat pos_x;
    GLfloat pos_y;
    GLfloat pos_z;
    // UV Data
    GLfloat texcoord_x;
    GLfloat texcoord_y;
    // Color Data
    GLuint abgr;
};
typedef TypedMesh<PosUVColorVertex> Mesh;
std::shared_ptr<Mesh> mouseMesh;

//==============================================================================
void renderTangram();

int main(int argc, char **argv){

    for (int i = 1; i < argc ; i++){
        if ( std::string(argv[i]) == "-m" ){
            bMouse = true;
        }
    }
    
    // Start OpenGL context
    initGL();

    // Prepair Mouse Shader
    if (bMouse) {
        mouseShader = std::shared_ptr<ShaderProgram>(new ShaderProgram());
        mouseShader->setSourceStrings(mouseFragment, mouseVertex );

        std::shared_ptr<VertexLayout> vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
            {"a_position", 3, GL_FLOAT, false, 0},
            {"a_texcoord", 2, GL_FLOAT, false, 0},
            {"a_color", 4, GL_UNSIGNED_BYTE, true, 0}
        }));

        std::vector<PosUVColorVertex> vertices;
        std::vector<int> indices;

        // Small billboard for the mouse
        GLuint color = 0xffffffff;
        {
            float x = -mouseSize*0.5f/state->screen_width;
            float y = -mouseSize*0.5f/state->screen_height;
            float w = mouseSize/state->screen_width;
            float h = mouseSize/state->screen_height;

            vertices.push_back({ x, y, 0.0, 0.0, 0.0, color});
            vertices.push_back({ x+w, y, 0.0, 0.0, 1.0, color});
            vertices.push_back({ x+w, y+h, 0.0, 1.0, 1.0, color});
            vertices.push_back({ x, y+h, 0.0, 0.0, 1.0, color });
            
            indices.push_back(0); indices.push_back(1); indices.push_back(2);
            indices.push_back(2); indices.push_back(3); indices.push_back(0);
        }
        
        mouseMesh = std::shared_ptr<Mesh>(new Mesh(vertexLayout, GL_TRIANGLES));
        mouseMesh->addVertices(std::move(vertices), std::move(indices));
        mouseMesh->compileVertexBuffer();
    }
    
    // Set background color and clear buffers
    Tangram::initialize();
    Tangram::resize(state->screen_width, state->screen_height);
    
    // Start clock
    gettimeofday(&tv, NULL);
    unsigned long long timePrev = (unsigned long long)(tv.tv_sec) * 1000 + (unsigned long long)(tv.tv_usec) / 1000;
    unsigned long long timeStart = (unsigned long long)(tv.tv_sec) * 1000 + (unsigned long long)(tv.tv_usec) / 1000; 

    while (bUpdate) {
        
        updateGL();

        if (getRenderRequest()) {
            renderTangram();
        } else {
            sleep(500);   
        }
    }
    
    Tangram::teardown();
    closeGL();
    return 0;
}

void renderTangram() {
    // Update
    unsigned long long timeNow = (unsigned long long)(tv.tv_sec) * 1000 + (unsigned long long)(tv.tv_usec) / 1000;
    double delta = (timeNow - timePrev)*0.001;
    float time = (timeNow - timeStart)*0.001;

    Tangram::update(delta);
    timePrev = timeNow;

    // Render        
    Tangram::render();

    if (bMouse) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST);
        mouseShader->use();
        mouseShader->setUniformf("u_time", time);
        mouseShader->setUniformf("u_mouse", mouse.x, mouse.y);
        mouseShader->setUniformf("u_resolution",state->screen_width, state->screen_height);
        mouseMesh->draw(mouseShader);
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
    }    

    updateGL();
}

//======================================================================= EVENTS

void onKeyPress(int _key) {
    if(key != -1){
        switch (key) {
            case KEY_ZOOM_IN:
                Tangram::handlePinchGesture(0.0,0.0,0.5);
                requestRender();
                break;
            case KEY_ZOOM_OUT:
                Tangram::handlePinchGesture(0.0,0.0,2.0);
                requestRender();
                break;
            case KEY_UP:
                Tangram::handlePanGesture(0.0,0.0,0.0,100.0);
                requestRender();
                break;
            case KEY_DOWN:
                Tangram::handlePanGesture(0.0,0.0,0.0,-100.0);
                requestRender();
                break;
            case KEY_LEFT:
                Tangram::handlePanGesture(0.0,0.0,100.0,0.0);
                requestRender();
                break;
            case KEY_RIGHT:
                Tangram::handlePanGesture(0.0,0.0,-100.0,0.0);
                requestRender();
                break;
            case KEY_ESC:
                bUpdate = false;
                break;
            default:
                logMsg(" -> %i\n",key);
        }   
    }
}

void onMouseMove(float _x, float _y) {
}

void onMouseClick(float _x, float _y, int _button) {

}

void onMouseDrag(float _x, float _y, int _button) {
    if( _button == 1 ){
        Tangram::handlePanGesture(  _x-getMouseVelX()*1.0, 
                                    _y+getMouseVelY()*1.0, 
                                    getMouseX(),
                                    getMouseY());
        requestRender();
    } else if( _button == 2 ){
        Tangram::handlePinchGesture( state->screen_width/2.0, state->screen_height/2.0, 1.0 + getMouseVelY()*0.001);
        requestRender();
    } 
}

void onViewportResize(int _newWidth, int _newHeight) {
    resizeViewport(_newWidth,_newHeight);
    requestRender();
}


