#pragma once

#include "gl.h"

// GL Context
void    initGL();
void    updateGL();
void    renderGL();
void    closeGL();

// SET
void    setWindowSize(int _width, int _height);
void    setRenderRequest(bool _request);

// GET
int     getWindowWidth();
int     getWindowHeight();
float   getMouseX();
float   getMouseY();
float   getMouseVelX();
float   getMouseVelY();
int     getMouseButton();
unsigned char getKeyPressed();
bool    getRenderRequest();

// EVENTS
void    onKeyPress(int _key);
void    onMouseMove(float _x, float _y);
void    onMouseClick(float _x, float _y, int _button);
void    onMouseDrag(float _x, float _y, int _button);
void    onViewportResize(int _width, int _height);
