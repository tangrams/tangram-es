#pragma once

/* LIGHTS 
-------------------------------
This openGL Lights implementation follows from the WebGL version of Tangram 
( https://github.com/tangrams/tangram/wiki/Lights-Overview )

Lights work together with the Materials to inject different lighting functions 
into the style shaders.
*/

/* Abstract Class */
#include "scene/light.h"

/* Lights Types */
#include "scene/ambientLight.h"
#include "scene/directionalLight.h"
#include "scene/pointLight.h"
#include "scene/spotLight.h"
