#include "context.h"

#include <assert.h>
#include <fcntl.h>
#include <iostream>
#include <termios.h>

#include "glm/gtc/matrix_transform.hpp"

#include "util/geom.h"
#include "string-conversions.h"

// Main global variables
//-------------------------------
#define check() assert(glGetError() == 0)
EGLDisplay display;
EGLSurface surface;
EGLContext context;

static glm::ivec4 viewport;
static glm::mat4 orthoMatrix;

typedef struct {
    float   x,y;
    float   velX,velY;
    int     button;
} Mouse;
static Mouse mouse;
static unsigned char keyPressed;

static bool bRender = true;

// OpenGL ES
//--------------------------------
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

    //  Adjust the viewport acording to the passed argument
    for (int i = 1; i < argc ; i++){
        if ( std::string(argv[i]) == "-x" ) {
            i++;
            viewport.x = getInt(std::string(argv[i]));
        } else if ( std::string(argv[i]) == "-y" ) {
            i++;
            viewport.y = getInt(std::string(argv[i]));
        } else if ( std::string(argv[i]) == "-w" || 
                    std::string(argv[i]) == "--width" ) {
            i++;
            viewport.z = getInt(std::string(argv[i]));
        } else if ( std::string(argv[i]) == "-h" || 
                    std::string(argv[i]) == "--height") {
            i++;
            viewport.w = getInt(std::string(argv[i]));
        } else if ( std::string(argv[i]) == "--square") {
            if (screen_width > screen_height) {
                viewport.x = screen_width/2-screen_height/2;
            } else {
                viewport.y = screen_height/2-screen_width/2;
            }
            viewport.z = viewport.w = MIN(screen_width,screen_height);
        } else if ( std::string(argv[i]) == "-l" || 
                    std::string(argv[i]) == "--life-coding" ){
            viewport.x = viewport.z-500;
            viewport.z = viewport.w = 500;
        }
    }

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
    check();

    ///printf("OpenGL Initialize at %i,%i,%i,%i\n",viewport.x,viewport.y,viewport.z,viewport.w);
}

void renderGL(){
    eglSwapBuffers(display, surface);
}

void closeGL(){
    eglSwapBuffers(display, surface);

    // Release OpenGL resources
    eglMakeCurrent( display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT );
    eglDestroySurface( display, surface );
    eglDestroyContext( display, context );
    eglTerminate( display );
}

// Inputs: Mouse/Keyboard
//--------------------------------
bool getMouse(){
    static int fd = -1;

    const int XSIGN = 1<<4, YSIGN = 1<<5;
    if (fd<0) {
        fd = open("/dev/input/mouse0",O_RDONLY|O_NONBLOCK);
    }
    if (fd>=0) {
        
        // Set values to 0
        mouse.velX=0;
        mouse.velY=0;
        
        // Extract values from driver
        struct {char buttons, dx, dy; } m;
        while (1) {
            int bytes = read(fd, &m, sizeof m);
            
            if (bytes < (int)sizeof m) {
                return false;
            } else if (m.buttons&8) {
                break; // This bit should always be set
            }
            
            read(fd, &m, 1); // Try to sync up again
        }
        
        // Set button value
        int button = m.buttons&3;
        
        // Lunch events
        if(button != mouse.button){
            mouse.button = button;
            if(mouse.button == 0){
                onMouseRelease(mouse.x,mouse.y);
            } else {
                onMouseClick(mouse.x,mouse.y,mouse.button);
            }
        }
        
        // Set deltas
        mouse.velX=m.dx;
        mouse.velY=m.dy;
        if (m.buttons&XSIGN) mouse.velX-=256;
        if (m.buttons&YSIGN) mouse.velY-=256;
        
        // Add movement
        mouse.x+=mouse.velX;
        mouse.y+=mouse.velY;
        
        // Clamp values
        if (mouse.x < 0) mouse.x=0;
        if (mouse.y < 0) mouse.y=0;
        if (mouse.x > viewport.z) mouse.x = viewport.z;
        if (mouse.y > viewport.w) mouse.y = viewport.w;


        if(mouse.velX != 0.0 || mouse.velY != 0.0){
            if (button != 0) {
                onMouseDrag(mouse.x,mouse.y,mouse.button);
            } else {
                onMouseMove(mouse.x,mouse.y);
            }
        }  

        return true;
    }
    return false;
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
    getMouse();

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

void setRenderRequest(bool _request) {
    bRender = _request;
}

bool getRenderRequest() {
    return bRender;
}