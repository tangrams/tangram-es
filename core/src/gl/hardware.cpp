#include "hardware.h"

#include "gl.h"
#include "platform.h"
#include "debug/textDisplay.h"

namespace Tangram {

namespace Hardware {

int maxTextureSize;

void loadCapability() {
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    LOGS("Hardware max texture size %d", maxTextureSize);
}

}

}
