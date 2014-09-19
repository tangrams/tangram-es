#pragma once

#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "platform.h"
#include "util/shaderProgram.h"
#include "util/vertexLayout.h"
#include "util/vboMesh.h"

void initializeOpenGL();
void resizeViewport(int newWidth, int newHeight);
void renderFrame();
