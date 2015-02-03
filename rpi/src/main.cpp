#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include <sys/shm.h>

#include <stdlib.h>
#include <string.h>
#include <termios.h>

#include "tangram.h"
#include "platform.h"
#include "gl.h"

#define KEY_ESC		27
#define KEY_ZOOM_IN 45
#define KEY_ZOOM_OUT 61
#define KEY_UP 		119
#define KEY_LEFT 	97
#define KEY_RIGHT 	115
#define KEY_DOWN 	122

struct timeval tv;

typedef struct {
    uint32_t screen_width;
    uint32_t screen_height;
    
    // OpenGL|ES objects
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    
} CUBE_STATE_T;

static CUBE_STATE_T _state, *state=&_state;

static void initOpenGL(){
    bcm_host_init();
    
    // Clear application state
    memset( state, 0, sizeof( *state ) );
    
    int32_t success = 0;
    EGLBoolean result;
    EGLint num_config;
    
    static EGL_DISPMANX_WINDOW_T nativewindow;
    
    DISPMANX_ELEMENT_HANDLE_T dispman_element;
    DISPMANX_DISPLAY_HANDLE_T dispman_display;
    DISPMANX_UPDATE_HANDLE_T dispman_update;
    VC_RECT_T dst_rect;
    VC_RECT_T src_rect;
    
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
    state->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    assert(state->display!=EGL_NO_DISPLAY);
    
    // initialize the EGL display connection
    result = eglInitialize(state->display, NULL, NULL);
    assert(EGL_FALSE != result);
    
    // get an appropriate EGL frame buffer configuration
    result = eglChooseConfig(state->display, attribute_list, &config, 1, &num_config);
    assert(EGL_FALSE != result);
    
    // get an appropriate EGL frame buffer configuration
    result = eglBindAPI(EGL_OPENGL_ES_API);
    assert(EGL_FALSE != result);
    
    // create an EGL rendering context
    state->context = eglCreateContext(state->display, config, EGL_NO_CONTEXT, context_attributes);
    assert(state->context!=EGL_NO_CONTEXT);
    
    // create an EGL window surface
    success = graphics_get_display_size(0 /* LCD */, &state->screen_width, &state->screen_height);
    assert( success >= 0 );
    
    dst_rect.x = 0;
    dst_rect.y = 0;
    dst_rect.width = state->screen_width;
    dst_rect.height = state->screen_height;
    
    src_rect.x = 0;
    src_rect.y = 0;
    src_rect.width = state->screen_width << 16;
    src_rect.height = state->screen_height << 16;
    
    dispman_display = vc_dispmanx_display_open( 0 /* LCD */);
    dispman_update = vc_dispmanx_update_start( 0 );
    dispman_element = vc_dispmanx_element_add ( dispman_update, dispman_display,
                                               0/*layer*/, &dst_rect, 0/*src*/,
                                               &src_rect, DISPMANX_PROTECTION_NONE, 0 /*alpha*/, 0/*clamp*/, 0/*transform*/);
    
    nativewindow.element = dispman_element;
    nativewindow.width = state->screen_width;
    nativewindow.height = state->screen_height;
    vc_dispmanx_update_submit_sync( dispman_update );
    
    
    state->surface = eglCreateWindowSurface( state->display, config, &nativewindow, NULL );
    assert(state->surface != EGL_NO_SURFACE);
    
    // connect the context to the surface
    result = eglMakeCurrent(state->display, state->surface, state->surface, state->context);
    assert(EGL_FALSE != result);
    
    // Set background color and clear buffers
    glClearColor(0.15f, 0.25f, 0.35f, 1.0f);
    glClear( GL_COLOR_BUFFER_BIT );
    
    // Prepare viewport
    glViewport( 0, 0, state->screen_width, state->screen_height );
}

typedef struct Mouse {
    Mouse():x(0),y(0),button(0){};
    
    float 	x,y;
    float 	velX,velY;
    int 	button;
};

static Mouse mouse;

static bool updateMouse(){
    static int fd = -1;
    const int width=state->screen_width, height=state->screen_height;
    //static int x=width, y=height;
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
        if (m.buttons&3)
            mouse.button = m.buttons&3;
        else
            mouse.button = 0;
        
        // Set deltas
        mouse.velX=m.dx;
        mouse.velY=m.dy;
        if (m.buttons&XSIGN) mouse.velX-=256;
        if (m.buttons&YSIGN) mouse.velY-=256;
        
        // Add movement
        mouse.x+=mouse.velX;
        mouse.y+=mouse.velY;
        
        // Clamp values
        if (mouse.x<0) mouse.x=0;
        if (mouse.y<0) mouse.y=0;
        if (mouse.x>width) mouse.x=width;
        if (mouse.y>height) mouse.y=height;
        return true;
    }
   	return false;
}

int getkey() {
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

//==============================================================================
int main(int argc, char **argv){
    
    // Start OpenGL context
    initOpenGL();
    
    // Set background color and clear buffers
    Tangram::initialize();
    Tangram::resize(state->screen_width, state->screen_height);
    
    // Start clock
    gettimeofday(&tv, NULL);
    unsigned long long timePrev = 	(unsigned long long)(tv.tv_sec) * 1000 +
    (unsigned long long)(tv.tv_usec) / 1000;
    
    while (1) {
        
        // Update
        unsigned long long timeNow = 	(unsigned long long)(tv.tv_sec) * 1000 +
        (unsigned long long)(tv.tv_usec) / 1000;
        double delta = (timeNow - timePrev)*0.001;
        
        Tangram::update(delta);
        timePrev = timeNow;
        
        if(updateMouse()){
            if( mouse.button == 1 ){
                Tangram::handleTapGesture( mouse.x, mouse.y );
            } else {
                Tangram::handlePanGesture( mouse.velX*10.0, -mouse.velY*10.0);
            }
        }

		int key = getkey();
		if(key != -1){
			switch (key) {
				case KEY_ZOOM_IN:
					Tangram::handlePinchGesture(0.0,0.0,0.5);
					break;
				case KEY_ZOOM_OUT:
					Tangram::handlePinchGesture(0.0,0.0,2.0);
					break;
				case KEY_UP:
					Tangram::handlePanGesture(0.0,100.0);
					break;
				case KEY_DOWN:
					Tangram::handlePanGesture(0.0,-100.0);
					break;
				case KEY_LEFT:
					Tangram::handlePanGesture(100.0,0.0);
					break;
				case KEY_RIGHT:
					Tangram::handlePanGesture(-100.0,0.0);
					break;
				default:
					logMsg(" -> %i\n",key);
			}	
		}

        // Render        
        Tangram::render();
        
        eglSwapBuffers(state->display, state->surface); 
    }
    
    Tangram::teardown();
    return 0;
}
