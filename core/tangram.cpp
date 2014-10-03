#include "tangram.h"

std::unique_ptr<SceneDirector> sceneDirector;

void initializeOpenGL()
{

    sceneDirector.reset(new SceneDirector());

    logMsg("%s\n", "initialize");

}

void resizeViewport(int newWidth, int newHeight)
{
    glViewport(0, 0, newWidth, newHeight);

    logMsg("%s\n", "resizeViewport");
}

void renderFrame()
{

    sceneDirector->update(0.016);

    logMsg("Updated\n");

    sceneDirector->renderFrame();

    logMsg("Rendered\n");
    
    GLenum glError = glGetError();
    if (glError) {
        logMsg("GL Error %d!!!\n", glError);
    }

}
