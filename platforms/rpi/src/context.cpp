#include "context.h"

#include <assert.h>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <fstream>
#include <termios.h>
#include <linux/input.h>

#include "glm/gtc/matrix_transform.hpp"
#include "platform.h"
#include "util/geom.h"

// Main global variables
//-------------------------------
#define check() assert(glGetError() == 0)
EGLDisplay display;
EGLSurface surface;
EGLContext context;

static glm::ivec4 viewport;
static glm::mat4 orthoMatrix;

#define MOUSE_ID "mouse0"
static int mouse_fd = -1;
typedef struct {
    float   x,y;
    float   velX,velY;
    int     button;
} Mouse;
static Mouse mouse;
static bool bRender;
static unsigned char keyPressed;

// Mouse stuff
//--------------------------------
std::string searchForDevice(const std::string& _device) {
    std::ifstream file;
    std::string buffer;
    std::string address = "NONE";

    file.open("/proc/bus/input/devices");

    if (!file.is_open())
      return "NOT FOUND";

    while (!file.eof()) {
      getline(file, buffer);
      std::size_t found = buffer.find(_device);
      if(found!=std::string::npos){
        std::string tmp = buffer.substr(found+_device.size()+1);
        std::size_t foundBegining = tmp.find("event");
        if(foundBegining!=std::string::npos){
            address = "/dev/input/"+tmp.substr(foundBegining);
            address.erase(address.size()-1,1);
        }
        break;
      }
    }

    file.close();
    return address;
}

void closeMouse(){
    if (mouse_fd > 0)
        close(mouse_fd);
    mouse_fd = -1;
}

int initMouse(){
    closeMouse();

    mouse.x = viewport.z*0.5;
    mouse.y = viewport.w*0.5;
    std::string mouseAddress = searchForDevice(MOUSE_ID);
    std::cout << "Mouse [" << mouseAddress << "]"<< std::endl;
    mouse_fd = open(mouseAddress.c_str(), O_RDONLY | O_NONBLOCK);

    return mouse_fd;
}


bool readMouseEvent(struct input_event *mousee){
    int bytes;
    if (mouse_fd > 0) {
        bytes = read(mouse_fd, mousee, sizeof(struct input_event));
        if (bytes == -1)
            return false;
        else if (bytes == sizeof(struct input_event))
            return true;
    }
    return false;
}

bool updateMouse() {
    if (mouse_fd < 0) {
        return false;
    }

    struct input_event mousee;
    while ( readMouseEvent(&mousee) ) {

        mouse.velX=0;
        mouse.velY=0;

        float x,y = 0.0f;
        int button = 0;

        switch(mousee.type) {
            // Update Mouse Event
            case EV_KEY:
                switch (mousee.code) {
                    case BTN_LEFT:
                        if (mousee.value == 1){
                            button = 1;
                        }
                        break;
                    case BTN_RIGHT:
                        if(mousee.value == 1){
                            button = 2;
                        }
                        break;
                    case BTN_MIDDLE:
                        if(mousee.value == 1){
                            button = 3;
                        }
                        break;
                    default:
                        button = 0;
                        break;
                }
                if (button != mouse.button) {
                    mouse.button = button;
                    if(mouse.button == 0){
                        onMouseRelease(mouse.x,mouse.y);
                    } else {
                        onMouseClick(mouse.x,mouse.y,mouse.button);
                    }
                }
                break;
            case EV_REL:
                switch (mousee.code) {
                    case REL_X:
                        mouse.velX = mousee.value;
                        break;
                    case REL_Y:
                        mousee.value = mousee.value * -1;
                        mouse.velY = mousee.value;
                        break;
                    // case REL_WHEEL:
                    //     if (mousee.value > 0)
                    //         std::cout << "Mouse wheel Forward" << std::endl;
                    //     else if(mousee.value < 0)
                    //         std::cout << "Mouse wheel Backward" << std::endl;
                    //     break;
                    default:
                        break;
                }
                mouse.x+=mouse.velX;
                mouse.y+=mouse.velY;

                // Clamp values
                if (mouse.x < 0) mouse.x=0;
                if (mouse.y < 0) mouse.y=0;
                if (mouse.x > viewport.z) mouse.x = viewport.z;
                if (mouse.y > viewport.w) mouse.y = viewport.w;

                if (mouse.button != 0) {
                    onMouseDrag(mouse.x,mouse.y,mouse.button);
                } else {
                    onMouseMove(mouse.x,mouse.y);
                }
                break;
            case EV_ABS:
                switch (mousee.code) {
                    case ABS_X:
                        x = ((float)mousee.value/4095.0f)*viewport.z;
                        mouse.velX = x - mouse.x;
                        mouse.x = x;
                        break;
                    case ABS_Y:
                        y = (1.0-((float)mousee.value/4095.0f))*viewport.w;
                        mouse.velY = y - mouse.y;
                        mouse.y = y;
                        break;
                    default:
                        break;
                }
                if (mouse.button != 0) {
                    onMouseDrag(mouse.x,mouse.y,mouse.button);
                } else {
                    onMouseMove(mouse.x,mouse.y);
                }
                break;
            default:
                break;
        }
    }
    return true;
}

// OpenGL ES
//--------------------------------

//==============================================================================
//  Creation of EGL context (lines 241-341) from:
//
//      https://github.com/raspberrypi/firmware/blob/master/opt/vc/src/hello_pi/hello_triangle2/triangle2.c#L100
//
/*
Copyright (c) 2012, Broadcom Europe Ltd
All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
//==============================================================================
void initGL(int argc, char **argv){

    // Start OpenGL ES
    bcm_host_init();

    // Clear application state
    int32_t success = 0;
    EGLBoolean result;
    EGLint num_config;

    static EGL_DISPMANX_WINDOW_T nativeviewport;

    DISPMANX_ELEMENT_HANDLE_T dispman_element;
    DISPMANX_DISPLAY_HANDLE_T dispman_display;
    DISPMANX_UPDATE_HANDLE_T dispman_update;
    VC_RECT_T dst_rect;
    VC_RECT_T src_rect;

    uint32_t screen_width;
    uint32_t screen_height;

    static const EGLint attribute_list[] = {
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_SAMPLE_BUFFERS, 1,
        EGL_SAMPLES, 4,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_DEPTH_SIZE, 16,
        EGL_NONE
    };

    static const EGLint context_attributes[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    EGLConfig config;

    // get an EGL display connection
    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    assert(display!=EGL_NO_DISPLAY);
    check();

    // initialize the EGL display connection
    result = eglInitialize(display, NULL, NULL);
    assert(EGL_FALSE != result);
    check();

    // get an appropriate EGL frame buffer configuration
    result = eglChooseConfig(display, attribute_list, &config, 1, &num_config);
    assert(EGL_FALSE != result);
    check();

    // get an appropriate EGL frame buffer configuration
    result = eglBindAPI(EGL_OPENGL_ES_API);
    assert(EGL_FALSE != result);
    check();

    // create an EGL rendering context
    context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_attributes);
    assert(context!=EGL_NO_CONTEXT);
    check();

    // create an EGL viewport surface
    success = graphics_get_display_size(0 /* LCD */, &screen_width, &screen_height);
    assert( success >= 0 );

    //  Initially the viewport is for all the screen
    viewport.x = 0;
    viewport.y = 0;
    viewport.z = screen_width;
    viewport.w = screen_height;

    dst_rect.x = viewport.x;
    dst_rect.y = viewport.y;
    dst_rect.width = viewport.z;
    dst_rect.height = viewport.w;

    src_rect.x = 0;
    src_rect.y = 0;
    src_rect.width = viewport.z << 16;
    src_rect.height = viewport.w << 16;

    dispman_display = vc_dispmanx_display_open( 0 /* LCD */);
    dispman_update = vc_dispmanx_update_start( 0 );

    dispman_element = vc_dispmanx_element_add( dispman_update, dispman_display,
                                       0/*layer*/, &dst_rect, 0/*src*/,
                                       &src_rect, DISPMANX_PROTECTION_NONE, 0 /*alpha*/, 0/*clamp*/, (DISPMANX_TRANSFORM_T)0/*transform*/);

    nativeviewport.element = dispman_element;
    nativeviewport.width = viewport.z;
    nativeviewport.height = viewport.w;
    vc_dispmanx_update_submit_sync( dispman_update );
    check();

    surface = eglCreateWindowSurface( display, config, &nativeviewport, NULL );
    assert(surface != EGL_NO_SURFACE);
    check();

    // connect the context to the surface
    result = eglMakeCurrent(display, surface, surface, context);
    assert(EGL_FALSE != result);
    check();

    // Set background color and clear buffers
    // glClearColor(0.15f, 0.25f, 0.35f, 1.0f);
    // glClear( GL_COLOR_BUFFER_BIT );

    setWindowSize(viewport.z,viewport.w);
    mouse.x = viewport.z*0.5;
    mouse.y = viewport.w*0.5;
    check();

    initMouse();
    ///printf("OpenGL Initialize at %i,%i,%i,%i\n",viewport.x,viewport.y,viewport.z,viewport.w);
}

void renderGL(){
    eglSwapBuffers(display, surface);
}

void closeGL(){
    closeMouse();
    eglSwapBuffers(display, surface);

    // Release OpenGL resources
    eglMakeCurrent( display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT );
    eglDestroySurface( display, surface );
    eglDestroyContext( display, context );
    eglTerminate( display );
}


int getKey() {
    int character;
    struct termios orig_term_attr;
    struct termios new_term_attr;

    /* set the terminal to raw mode */
    tcgetattr(fileno(stdin), &orig_term_attr);
    memcpy(&new_term_attr, &orig_term_attr, sizeof(struct termios));
    new_term_attr.c_lflag &= ~(ECHO|ICANON);
    new_term_attr.c_cc[VTIME] = 0;
    new_term_attr.c_cc[VMIN] = 0;
    tcsetattr(fileno(stdin), TCSANOW, &new_term_attr);

    /* read a character from the stdin stream without blocking */
    /*   returns EOF (-1) if no character is available */
    character = fgetc(stdin);

    /* restore the original terminal attributes */
    tcsetattr(fileno(stdin), TCSANOW, &orig_term_attr);

    return character;
}

void updateGL() {
    updateMouse();

    int key = getKey();
    if ( key != -1 && key != keyPressed ){
        keyPressed = key;
        onKeyPress(key);
    }
}

void setWindowSize(int _width, int _height) {
    viewport.z = _width;
    viewport.w = _height;
    glViewport((float)viewport.x, (float)viewport.y, (float)viewport.z, (float)viewport.w);
    orthoMatrix = glm::ortho((float)viewport.x, (float)viewport.z, (float)viewport.y, (float)viewport.w);

    onViewportResize(viewport.z, viewport.w);
}

int getWindowWidth(){
    return viewport.z;
}

int getWindowHeight(){
    return viewport.w;
}

glm::vec2 getWindowSize() {
    return glm::vec2(viewport.w,viewport.w);
}

glm::mat4 getOrthoMatrix(){
    return orthoMatrix;
}

float getMouseX(){
    return mouse.x;
}

float getMouseY(){
    return mouse.y;
}

glm::vec2 getMousePosition() {
    return glm::vec2(mouse.x,mouse.y);
}

float getMouseVelX(){
    return mouse.velX;
}

float getMouseVelY(){
    return mouse.velY;
}

glm::vec2 getMouseVelocity() {
    return glm::vec2(mouse.velX,mouse.velY);
}

int getMouseButton(){
    return mouse.button;
}

unsigned char getKeyPressed(){
    return keyPressed;
}

void setRenderRequest(bool _render) {
    bRender = _render;
}

bool getRenderRequest() {
    return bRender;
}
