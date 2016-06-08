#pragma once

#include "common/platform_common.h"
#undef countof
#include "glm/glm.hpp"

// GL Context
void    initGL(int argc, char **argv);
void    updateGL();
void    renderGL();
void    closeGL();

// SET
void    setWindowSize(int _width, int _height);
void    setRenderRequest(bool _request);

// GET
int     getWindowWidth();
int     getWindowHeight();
glm::vec2 getWindowSize();
glm::mat4 getOrthoMatrix();

float   getMouseX();
float   getMouseY();
glm::vec2 getMousePosition();

float   getMouseVelX();
float   getMouseVelY();
glm::vec2 getMouseVelocity();

int     getMouseButton();

unsigned char getKeyPressed();

bool    getRenderRequest();

// EVENTS
void    onKeyPress(int _key);
void    onMouseMove(float _x, float _y);
void    onMouseClick(float _x, float _y, int _button);
void    onMouseDrag(float _x, float _y, int _button);
void    onMouseRelease(float _x, float _y);
void    onViewportResize(int _width, int _height);
