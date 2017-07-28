#pragma once

#include "platform_gl.h"

// GL Context
void createSurface(int x, int y, int width, int height);
void pollInput();
void swapSurface();
void destroySurface();

// SET
void setWindowSize(int width, int height);
void setRenderRequest(bool render);

// GET
bool getRenderRequest();
int getWindowWidth();
int getWindowHeight();

float getMouseX();
float getMouseY();

float getMouseVelX();
float getMouseVelY();

int getMouseButton();

unsigned char getKeyPressed();

// EVENTS
void onKeyPress(int key);
void onMouseMove(float x, float y);
void onMouseClick(float x, float y, int button);
void onMouseDrag(float x, float y, int button);
void onMouseRelease(float x, float y);
void onViewportResize(int width, int height);
