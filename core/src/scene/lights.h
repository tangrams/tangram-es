#pragma once

/* LIGTHS 
-------------------------------
This openGL Lights implementation mimics the behavior of 
the WebGL version of Tangram ( https://github.com/tangrams/tangram/wiki/Lights-Overview )

Works together with the Material Class to inject different lights and functions in to the 
style shader.
*/

/* Abstract Class */
#include "scene/light.h"

/* Lights Types */
#include "scene/ambientLight.h"
#include "scene/directionalLight.h"
#include "scene/pointLight.h"
#include "scene/spotLight.h"